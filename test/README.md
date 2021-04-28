w4118@ubuntu:~/hmw4/test$ sudo ./syscall_test
[ info ] Setting process 2285 to use wrr_sched_class...
[ success ] pid 2285 set to use policy 7
[ success ] denied request to set weight < 1
Enter weight: 2
[ success ] set pid 2285's wrr_weight to 2
[ success ] num_cpus: 8                                     # starts syscall_test
[ info ] cpu_0  nr_running: 1           total_weight 2
[ info ] cpu_1  nr_running: 1           total_weight 1
[ info ] cpu_2  nr_running: 0           total_weight 0
[ info ] cpu_3  nr_running: 0           total_weight 0
[ info ] cpu_4  nr_running: 0           total_weight 0
[ info ] cpu_5  nr_running: 0           total_weight 0
[ info ] cpu_6  nr_running: 0           total_weight 0
[ info ] cpu_7  nr_running: 0           total_weight 0
Enter weight: 10
[ success ] set pid 2285's wrr_weight to 10
[ success ] num_cpus: 8                                     # starts 4 while(1) processes
[ info ] cpu_0  nr_running: 1           total_weight 1
[ info ] cpu_1  nr_running: 1           total_weight 1
[ info ] cpu_2  nr_running: 2           total_weight 2
[ info ] cpu_3  nr_running: 1           total_weight 1
[ info ] cpu_4  nr_running: 1           total_weight 10
[ info ] cpu_5  nr_running: 1           total_weight 1
[ info ] cpu_6  nr_running: 0           total_weight 0
[ info ] cpu_7  nr_running: 0           total_weight 0
Enter weight: 20
[ success ] set pid 2285's wrr_weight to 20
[ success ] num_cpus: 8                                   # 4 while(1) processes are killed
[ info ] cpu_0  nr_running: 1           total_weight 20
[ info ] cpu_1  nr_running: 1           total_weight 1
[ info ] cpu_2  nr_running: 1           total_weight 1
[ info ] cpu_3  nr_running: 0           total_weight 0
[ info ] cpu_4  nr_running: 0           total_weight 0
[ info ] cpu_5  nr_running: 0           total_weight 0
[ info ] cpu_6  nr_running: 0           total_weight 0
[ info ] cpu_7  nr_running: 0           total_weight 0
Enter weight: 30
[ success ] set pid 2285's wrr_weight to 30
[ success ] num_cpus: 8                                   # starts 16 while(1) processes and vim
[ info ] cpu_0  nr_running: 3           total_weight 3
[ info ] cpu_1  nr_running: 3           total_weight 3
[ info ] cpu_2  nr_running: 3           total_weight 3
[ info ] cpu_3  nr_running: 4           total_weight 33
[ info ] cpu_4  nr_running: 3           total_weight 3
[ info ] cpu_5  nr_running: 2           total_weight 2
[ info ] cpu_6  nr_running: 3           total_weight 3
[ info ] cpu_7  nr_running: 2           total_weight 2
Enter weight: 1
[ success ] set pid 2285's wrr_weight to 1
[ success ] num_cpus: 8                                  # killed 16 while(1) processes and vim
[ info ] cpu_0  nr_running: 1           total_weight 1
[ info ] cpu_1  nr_running: 1           total_weight 1
[ info ] cpu_2  nr_running: 0           total_weight 0
[ info ] cpu_3  nr_running: 0           total_weight 0
[ info ] cpu_4  nr_running: 0           total_weight 0
[ info ] cpu_5  nr_running: 0           total_weight 0
[ info ] cpu_6  nr_running: 0           total_weight 0
[ info ] cpu_7  nr_running: 0           total_weight 0
