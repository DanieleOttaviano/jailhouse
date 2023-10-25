import seaborn as sns
import matplotlib.pyplot as plt
import statistics

frequency=100000
# Function to read and parse the data from a file
def parse_data(file_name,until):
    data = []
    with open(file_name, 'r') as file:
        lines = file.readlines()
        lines = [line for line in lines if "remoteproc" not in line]
        i = 0


        while i < len(lines):
            line = lines[i].strip()
            if line.startswith("Image Size:"):
                image_size = int(line.split()[-1])
                i += 1
                init_times = []
                create_times = []
                load_times = []
                start_times = []
                end_times = []
                while i < len(lines):
                    line = lines[i].strip()
                    if line.startswith("init_time:"):
                        init=int(line.split()[-1], 16)
                        init_times.append(init)
                    elif line.startswith("create_time:"):
                        create = int(line.split()[-1], 16)
                        create_times.append(create)
                    elif line.startswith("load_time:"):
                        load = int(line.split()[-1], 16)
                        load_times.append(load)
                    elif line.startswith("start_time:"):
                        start = int(line.split()[-1], 16)
                        start_times.append(start)
                    elif line.startswith("end_time:"):
                        end = int(line.split()[-1], 16) 
                        end_times.append(end)
                    else:
                        # If a new "Image Size" line is encountered, calculate the mean time difference
                        mean_boot_time = statistics.mean([end - start if end > start else  end + (int(0xFFFFFFFF) - start) for start, end in zip(init_times, end_times)])/frequency
                        mean_create_time = statistics.mean([end - start if end > start else  end + (int(0xFFFFFFFF) - start) for start, end in zip(init_times, create_times)])/frequency
                        mean_load_time = statistics.mean([end - start if end > start else  end + (int(0xFFFFFFFF) - start) for start, end in zip(init_times, load_times)])/frequency
                        mean_start_time = statistics.mean([end - start if end > start else  end + (int(0xFFFFFFFF) - start) for start, end in zip(init_times, start_times)])/frequency
                        
                        if until == "end":
                            data.append((mean_boot_time, image_size))
                        elif until == "create":
                            data.append((mean_create_time, image_size))
                        elif until == "load":
                            data.append((mean_load_time, image_size))
                        elif until == "start":
                            data.append((mean_start_time, image_size))
                        break
                    i += 1            
            i += 1
    return data

# File names
file1 = "boot_times_omnivisor.txt"
file2 = "boot_times_remoteproc.txt"

# Parse data from the files
data1 = parse_data(file1,"create")
data2 = parse_data(file1,"load")
data3 = parse_data(file1,"start")
data4 = parse_data(file1,"end")
data5 = parse_data(file2,"end")

print(data1)
print(data2)
print(data3)
print(data5)

# Create a Seaborn plot
sns.set(style="whitegrid")
plt.figure(figsize=(10, 6))

# Plot data from the first file
sns.lineplot(x=[x[1] for x in data1], y=[x[0] for x in data1], label="create times omnivisor") #, color="black"

# Plot data from the second file
sns.lineplot(x=[x[1] for x in data2], y=[x[0] for x in data2], label="load times omnivisor")

# Plot data from the first file
sns.lineplot(x=[x[1] for x in data3], y=[x[0] for x in data3], label="start times omnivisor") #, color="black"

# Plot data from the second file
sns.lineplot(x=[x[1] for x in data4], y=[x[0] for x in data4], label="boot times omnivisor")

# Plot data from the second file
sns.lineplot(x=[x[1] for x in data5], y=[x[0] for x in data5], label="boot times remoteproc", color="black")


plt.ylabel("Time (ms)")
plt.xlabel("VM Image Size (Mb)")
plt.title("Boot Times")
plt.legend()

plt.show()
                                                       
