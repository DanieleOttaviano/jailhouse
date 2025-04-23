#include <inmate.h>

#define SWITCH 5000

struct shared_page {
    /*
        DATA Structure
    */
    struct data {
        /* 0x000 */
        double pf_voltage[11];
        /* 0x058 */
        double vs3;
        /* 0x060 */
        double pf_currents[11];
        /* 0x0B8 */
        double ivs3;
        /* 0x0C0 */
        double ip;
        /* 0x0C8 */
        double zc;
        /* 0x0D0 */
        double zdot;
        /* 0xD8 */
        double gaps[29];
        /* 0x1C0 */
        double Xpoints[2];
    } data;
    
    /* 0x1D0 */
    u8 oracle_decision;
    /* 0x1D1 */
    u8 input_counter;
    /* 0x1D2 */
    u8 marte_strategy;
    /* 0x1D3 */
    u8 output_counter;

    /* 0x1D4 */
    u32 reserved;

    /* 0x1D8 */
    double es_output;
    /* 0x1E0 */
    double classic_output;
    /* 0x1E8*/
    double vs3_ref;
};

void inmate_main(void)
{
	volatile struct shared_page* shared_memory  = (struct shared_page*) 0x46d00000;
	u8 last_counter;
	double tmp0, tmp1, tmp2;

	tmp0 = 0;
	tmp1 = 1;
	tmp2 = 2;

	map_range((void *)shared_memory, sizeof(struct shared_page), MAP_UNCACHED);

	last_counter = shared_memory->input_counter;

    printk("Starting Oracle\n");
	while (1) {
		if (last_counter != shared_memory->input_counter) {
			for (int i = 0; i < 29; i++) {
				tmp2 = tmp0 * shared_memory->data.ip + tmp2 * shared_memory->data.gaps[i];
				tmp0 = shared_memory->data.gaps[i] + tmp1 * shared_memory->data.gaps[i / 2];
				tmp1 += i * shared_memory->data.vs3;
			}

            if( shared_memory->input_counter >= (u8)SWITCH && shared_memory->oracle_decision == 0) {
                printk("##########SWITCH#########\n");
				shared_memory->oracle_decision = 0x01;
                while(1);
			}
		}
	}
}