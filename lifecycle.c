#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int ack(int m, int n) 
{ 
	if (m == 0){ 
		return n+1; 
	} 
	else if((m > 0) && (n == 0)){ 
		return ack(m-1, 1); 
	} 
	else if((m > 0) && (n > 0)){ 
		return ack(m-1, ack(m, n-1)); 
	} 
	return n-1;
} 

int main(){
	int file = open("./lifecycle.c", __O_LARGEFILE);
	char buf[10];
	int i;

	for (i = 0; i < 10; i++){
		ack(3,8);
		sleep(3);
		read(file, buf, sizeof(buf));
	}

	close(file);
	return 0;
}
