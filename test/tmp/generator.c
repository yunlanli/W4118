#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(){
	char buf[50];
	int sys_ret;
	int pid = getpid();
	printf("pid= %d", pid);
	
	sys_ret = syscall(436, pid);
	if (sys_ret)
		printf("pstrace_enable returned: %d\n", sys_ret);

	while(1){
		printf("Enter something: ");
		scanf("%s\n", buf);
	}
}
