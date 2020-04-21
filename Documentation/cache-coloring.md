Cache Coloring Support
======================

Introduction
------------

### Cache partitioning and coloring

#### Motivation

Cache hierarchies of modern multi-core CPUs typically have first levels dedicated
to each core (hence using multiple cache units), while the last level cache
(LLC) is shared among all of them. Such configuration implies that memory
operations on one core, e.g., running one Jailhouse inmate, are able to generate
timing *interference* on another core, e.g., hosting another inmate. More
specifically, data cached by the latter core can be evicted by cache store
operations performed by the former. In practice, this means that the memory
latency experienced by one core depends on the other cores (in-)activity.

The obvious solution is to provide hardware mechanisms allowing either: a
fine-grained control with cache lock-down, as offered on the previous v7
generation of Arm architectures; or a coarse-grained control with LLC
partitioning among different cores, as featured on the "Cache Allocation
Technology" of the high-end segment of recent Intel architecture and supported
by the Jailhouse hypervisor.

#### Cache coloring

Cache coloring is a *software technique* that permits LLC partitioning,
therefore eliminating mutual core interference, and thus guaranteeing higher and
more predictable performances for memory accesses. A given memory space in
central memory is partitioned into subsets called colors, so that addresses in
different colors are necessarily cached in different LLC lines. On Arm
architectures, colors are easily defined by the following circular striding.

```
          _ _ _______________ _ _____________________ _ _
               |     |     |     |     |     |     |
               | c_0 | c_1 |     | c_n | c_0 | c_1 |
          _ _ _|_____|_____|_ _ _|_____|_____|_____|_ _ _
                  :                       :
                  '......         ........'
                        . color 0 .
                . ........      ............... .
                         :      :
            . ...........:      :..................... .
```

Cache coloring suffices to define separate domains that are guaranteed to be
*free from interference* with respect to the mutual evictions, but it does not
protect from minor interference effects still present on LLC shared
subcomponents (almost negligible), nor from the major source of contention
present in central memory.

It is also worth remarking that cache coloring also partitions the central
memory availability accordingly to the color allocation--assigning, for
instance, half of the LLC size is possible if and only if half of the DRAM space
is assigned, too.


### Cache coloring in Jailhouse

The *cache coloring support in Jailhouse* allows partitioning the cache by
simply partitioning the colors available on the specific platform, whose number
may vary depending on the specific cache implementation. More detail about color
availability and selection is provided in [Usage](#usage).

#### Supported architectures

Cache coloring is available on Arm64 architectures. In particular, extensive
testing has been performed on v8 CPUs, namely on the A53 and A57 processors
equipping Xilinx ZCU102 and ZCU104.

### Further readings

Relevance, applicability, and evaluation results of the Jailhouse cache coloring
support are reported in several recent works. A non-technical perspective is
given in [1] together with an overview of the ambitious HERCULES research
project. A technical and scientific presentation is instead authored in [2],
where additional experimental techniques on cache and DRAM are introduced.

An enjoyable, comprehensive and up-to-date survey on cache management technique
for real-time systems is offered by [3].

1. P. Gai, C. Scordino, M. Bertogna, M. Solieri, T. Kloda, L. Miccio. 2019.
   "Handling Mixed Criticality on Modern Multi-core Systems: the HERCULES
   Project", Embedded World Exhibition and Conference 2019.

2. T. Kloda, M. Solieri, R. Mancuso, N. Capodieci, P. Valente, M. Bertogna.
   2019.
   "Deterministic Memory Hierarchy and Virtualization for Modern Multi-Core
   Embedded Systems", 25th IEEE Real-Time and Embedded Technology and
   Applications Symposium (RTAS'19). To appear.

3. G. Gracioli, A. Alhammad, R. Mancuso, A.A. Froehlich, and R. Pellizzoni. 2015.
   "A Survey on Cache Management Mechanisms for Real-Time Embedded Systems", ACM
   Comput. Surv. 48, 2, Article 32 (Nov. 2015), 36 pages. DOI:10.1145/2830555




Usage
-----

### Colors selection

In order to choose a color assignment for a set of inmates, the first thing we
need to know is... the available color set. The number of available colors can
be either calculated or read from the output given by Jailhouse once
we enable the hypervisor.

To compute the number of available colors on the platform one can simply
divide
`way_size` by `page_size`, where: `page_size` is the size of the page used
on the system (usually 4 KiB); `way_size` is size of a LLC way, i.e. the same
value that has to be provided in the root cell configuration.
E.g., 16 colors on a platform with LLC ways sizing 64 KiB and 4 KiB pages.

Once the number of available colors (N) is known, the range of colors to be
associated to a memory region (see [cells configuration](#cells-configuration))
can be specified as a bitmask where contiguous bits specify a color range.
E.g., if 16 colors are available, a color bitmask `0xffff` corresponds to the
full color palette i.e., the full `way_size`, while a color bitmask `0x000f`
selects only 4 colors for the inmate.

#### Partitioning

We can choose any kind of color configuration we want but in order to have
mutual cache protection between cells, different colors must be assigned to them.
Another point to remember is to keep colors as contiguous as possible, so to
allow caches to exploit the higher performance to central memory controller.

### Root Cell configuration

#### LLC way size

The LLC way size can be specified as parameter `way_size` in the
`struct jailhouse_coloring` `color` structure of `platform_info`.
Currently, if `way_size` is not specified, the system will compute its value at
enable-time.

#### Temporary load-remapping address

When inmates use cache-coloring, a temporary load address is used to facilitate
coloring of the inmates during load-mapping via the root cell. The start address
of this temporary region can be provided via the `root_map_offset` parameter
in the `color` structure of `platform_info`.

For example, a 16-way set associative cache sizing 1 MiB has a way size of
64 KiB, and the the temporary load-remapping address is set to 0x0C000000000.
```
...
.platform_info = {
    ...
    .color = {
        /* autodetected if not specified */
        /* .way_size = 0x10000, */
        .root_map_offset = 0x0C000000000,
    },
    ...
```

### Cells configuration

A colored memory region is identified with the flag `JAILHOUSE_MEM_COLORED`.
The color bitmask specified in `.colors` identifies the colors associated with
the region, i.e., the `colors` bitmask applies to the mappings of the
`mem_region` entry. Different `mem_region` entries in the same cell may have
different colors. Coloring a `mem_region` doesn't change the specified `size`.
```
...
struct jailhouse_memory mem_regions[12];
...
.num_memory_regions = ARRAY_SIZE(config.mem_regions)
...
.mem_regions = {
    ...
    {
            .phys_start = 0x801100000,
            .virt_start = 0,
            .size = 0x10000,
            .flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
                    JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE |
                    JAILHOUSE_MEM_COLORED,
            /* Assigning 1/4 of the colors */
            .colors=0x000f,
    },
    ...
}
...
```
#### Overlaps and colored memory sizes

When using colored memory regions the rule `phys_end = phys_start + size` is no
longer true. So the configuration must be written carefully in order to avoid to
exceed the available memory in the root cell.
Moreover, since the above rule does not apply, it is very common to have overlaps
between colored memory regions of different cells if they are sharing colors.
