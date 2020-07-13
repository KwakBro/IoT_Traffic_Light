#include <stdio.h>
#include <wiringPi.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#define DOWN_RED 15
#define DOWN_BLUE 16
#define DOWN_GREEN 1
#define DOWN_BUTTON1 4
#define DOWN_BUTTON2 5
#define RIGHT_RED 6
#define RIGHT_BLUE 10
#define RIGHT_GREEN 11
#define RIGHT_BUTTON1 27 //31
#define RIGHT_BUTTON2 26
#define LEFT_RED 8
#define LEFT_BLUE 9
#define LEFT_GREEN 7
#define LEFT_BUTTON1 0
#define LEFT_BUTTON2 2
#define UP_RED 12
#define UP_BLUE 13
#define UP_GREEN 14
#define UP_BUTTON1 29  //30
#define UP_BUTTON2 28 //21
#define BUSY_T 1000000
#define TIME 5000
#define TTIME 7000
#define tIME 3000
#define True 1
#define False 0
#define BUFF_SIZE 1024
#define PORT 9999
#define SIZE 17
struct sockaddr_in addr, client_addr;
char recv_buffer[1024];
int recv_len;
int addr_len;
int what_node_busy[8] = { 0, };
 
 
int   server_addr_size;
struct sockaddr_in   server_addr;
struct sockaddr_in   client_addr;
char MODE;
int Left_Time;
char STATE;
int t_time;
enum B_state { NOT_BUSY, BUSY };
enum B_state D_1, D_2, R_1, R_2, U_1, U_2, L_1, L_2;
typedef struct {
	int pin;
    int now_busy;
    int sock;
   
} send_buff;    //////////////////////////////////
typedef struct {
  int node_num;
} argument;   ///////////////////////////////////
void light(int DOWN, int RIGHT, int UP, int LEFT, int time)
{
	pinMode(DOWN, OUTPUT);
	pinMode(RIGHT, OUTPUT);
	pinMode(UP, OUTPUT);
	pinMode(LEFT, OUTPUT);
	digitalWrite(DOWN, HIGH);
	digitalWrite(RIGHT, HIGH);
	digitalWrite(UP, HIGH);
	digitalWrite(LEFT, HIGH);
	delay(time);
	digitalWrite(DOWN, LOW);
	digitalWrite(RIGHT, LOW);
	digitalWrite(UP, LOW);
	digitalWrite(LEFT, LOW);
}
void light_A(int time)
{
	MODE = 'A';
	printf("%c MODE, TIME : %d\n", MODE,time);
	light(DOWN_GREEN, RIGHT_RED, UP_GREEN, LEFT_RED, time);
}
void light_B(int time)
{
	MODE = 'B';
	printf("%c MODE, TIME : %d\n", MODE,time);
	light(DOWN_RED, RIGHT_GREEN, UP_RED,LEFT_GREEN, time);
}
void light_C(int time)
{
	MODE = 'C';
	pinMode(DOWN_RED, OUTPUT);
	pinMode(RIGHT_RED, OUTPUT);
	printf("%c MODE, TIME : %d\n", MODE,time);
	digitalWrite(DOWN_RED, HIGH);
	digitalWrite(UP_RED, HIGH);
	light(DOWN_BLUE, RIGHT_RED, UP_BLUE,LEFT_RED, time);
	digitalWrite(DOWN_RED, LOW);
	digitalWrite(UP_RED, LOW);
}
void light_D(int time)
{
	MODE = 'D';
	pinMode(RIGHT_RED, OUTPUT);
	pinMode(LEFT_RED, OUTPUT);
	printf("%c MODE, TIME : %d\n", MODE,time);
	digitalWrite(RIGHT_RED, HIGH);
	digitalWrite(LEFT_RED, HIGH);
	light(DOWN_RED, RIGHT_BLUE, UP_RED,LEFT_BLUE, time);
	digitalWrite(RIGHT_RED, LOW);
	digitalWrite(LEFT_RED, LOW);
}
typedef struct ARG
{
	int pin;
	enum B_state temp;
}ARG;


 
void* traffic_light(void * arg)
{   //this function means control traffic light
	int next_small = 0 , next_long =0;
	
	for (;;)
	{
		if ( (D_2 == BUSY || U_2 == BUSY) && (U_1 != BUSY && D_1 != BUSY  ) )
		{	// only pattern A BUSY
			t_time = TTIME;
			light_A(t_time);
			D_2 = U_2= NOT_BUSY;
			next_small++;
	    }
	    else if( (D_1 == BUSY || U_1 == BUSY) && (U_2 != BUSY && D_2 != BUSY  ) )	// only pattern C ( next of A ) BUSY
		  {
			  	t_time = tIME;
			  	light_A(t_time);
				next_small--;
		  }
		else 
		{	// pattern A all clear 
			t_time = TIME;
			if(next_small > 0){
				t_time = tIME;
				next_small--;
			}		// if pattern needs smaller time 
			light_A(t_time);
		}
		
		if( (D_1 == BUSY || U_1 == BUSY) && (U_2 != BUSY && D_2 != BUSY  ) )
		{	// only pattern C BUSY
			t_time = TTIME;
			light_C(t_time);
			D_1 = U_1= NOT_BUSY;
			next_small++;
		}
		else
		{	// pattern C all clear
			t_time = TIME;

			if(next_small > 0){
				t_time = tIME;
				next_small--;
			}
			light_C(TIME);
		}

		if ( (R_2 == BUSY || L_2 == BUSY || what_node_busy[1] == BUSY || what_node_busy[5] == BUSY)
		  && (L_1 != BUSY && R_1 != BUSY )   )
		{	// only pattern B BUSY
			t_time = TTIME;
			light_B(t_time);
			R_2 = L_2= NOT_BUSY;
			if(what_node_busy[1] == BUSY){
				what_node_busy[1] = NOT_BUSY;
			}
			if(what_node_busy[5] == BUSY){
				what_node_busy[5] = NOT_BUSY;
			}
			next_small++;
		}
		else if( (R_1 == BUSY || L_1 == BUSY) && (R_2 != BUSY && L_2 != BUSY  ) )	// only pattern D ( next of B ) BUSY
		  {
			  	t_time = tIME;	  	
			  	light_B(t_time);	
				next_small--;
		  }
		else 
		{	// pattern B all NOT BUSY 
			t_time = TIME;
			light_B(t_time);
		}


		if ( (R_1 == BUSY || L_1 == BUSY) && (U_2 != BUSY && D_2 != BUSY  ) )
		{	// only pattern D BUSY
			t_time = TTIME;	
			light_D(t_time);
			R_1 = L_1= NOT_BUSY;
			next_small++;
		}
		else
		{	// pattern D all clear 
			t_time = TIME;
			if(next_small > 0){
				t_time = tIME;
				next_small--;
			}
			light_D(t_time);
		}
	}

}

void* busy_check(void* arg)	//button nulim thread
{

	send_buff *my_arg = (send_buff *)malloc(sizeof(send_buff));
	my_arg = (send_buff*)arg;

	int BUTTON = my_arg->pin;
	int sock = my_arg->sock;
	int i = 0;
	for( ;; )
	{
		if( digitalRead(BUTTON) == 1 )	//pressed down
		{
			if ( BUTTON == DOWN_BUTTON1 )
			{ 
				i++;
				if ( i % 25000 == 0 ) {delay(50); printf("D1 wait... : %d\n", (i/100000));}
				if ( i >= BUSY_T )
				{					
					D_1 = my_arg->now_busy = BUSY;
					printf("[[[  D_1 = BUSY!  ]]]\n");
					printf("my_arg->now_busy : %d\n", my_arg->now_busy);
					printf("sending data : %d, %d\n", my_arg->pin, my_arg->now_busy);
					i = 0 ;		// to zero when pattern over
					sendto(sock, my_arg, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr, sizeof(server_addr));
					delay(500);
				}
			}
			if ( BUTTON == DOWN_BUTTON2 )
			{ 
				i++;
				if ( i % 25000 == 0 ) {delay(50); printf("D2 wait... : %d\n", (i/100000));}
				if ( i >= BUSY_T )
				{
					D_2 = my_arg->now_busy = BUSY;
					printf("[[[  D_2 = BUSY!  ]]]\n");
					printf("my_arg->now_busy : %d\n", my_arg->now_busy);
					printf("sending data : %d, %d, %d\n", my_arg->pin, my_arg->now_busy, sizeof(send_buff));
					i = 0 ;
					sendto(sock, my_arg, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr, sizeof(server_addr));
					delay(500);
				}
			}
			if ( BUTTON == RIGHT_BUTTON1 )
			{
				i++;
				if ( i % 25000 == 0 ) {delay(50); printf("R1 wait... : %d\n", (i/100000));}
				if ( i >= BUSY_T )
				{
					R_1 = my_arg->now_busy = BUSY;
					printf("[[[  R_1 = BUSY!  ]]]\n");
					printf("my_arg->now_busy : %d\n", my_arg->now_busy);
					printf("sending data : %d, %d\n", my_arg->pin, my_arg->now_busy);
					i = 0 ;
					sendto(sock, my_arg, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr, sizeof(server_addr));
					delay(500);
				}
			}
			if ( BUTTON == RIGHT_BUTTON2 )
			{ 
				i++;
				if ( i % 25000 == 0 ) {delay(50); printf("R2 wait... : %d\n", (i/100000));}
				if ( i >= BUSY_T )
				{
					R_2 = my_arg->now_busy = BUSY;
					printf("[[[  R_2 = BUSY!  ]]]\n");
					printf("my_arg->now_busy : %d\n", my_arg->now_busy);
					printf("sending data : %d, %d\n", my_arg->pin, my_arg->now_busy);
					i = 0 ;
					sendto(sock, my_arg, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr, sizeof(server_addr));
					delay(500);
				}
			}
			if ( BUTTON == UP_BUTTON1 )
			{ 
				i++;
				if ( i % 25000 == 0 ) {delay(50); printf("U1 wait... : %d\n", (i/100000));}
				if ( i >= BUSY_T )
				{
					U_1 = my_arg->now_busy = BUSY;
					printf("[[[  U_1 = BUSY!  ]]]\n");
					printf("my_arg->now_busy : %d\n", my_arg->now_busy);
					printf("sending data : %d, %d\n", my_arg->pin, my_arg->now_busy);
					i = 0 ;
					sendto(sock, my_arg, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr, sizeof(server_addr));
					delay(500);
				}
			}
			if ( BUTTON == UP_BUTTON2 )
			{ 
				i++;
				if ( i % 25000 == 0 ) {delay(50); printf("U2 wait... : %d\n", (i/100000));}
				if ( i >= BUSY_T )
				{
					U_2 = my_arg->now_busy = BUSY;
					printf("[[[  U_2 = BUSY!  ]]]\n");
					printf("my_arg->now_busy : %d\n", my_arg->now_busy);
					printf("sending data : %d, %d\n", my_arg->pin, my_arg->now_busy);
					i = 0 ;
					sendto(sock, my_arg, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr, sizeof(server_addr));
					delay(500);
				}
			}
			if ( BUTTON == LEFT_BUTTON1 )
			{ 
				i++;
				if ( i % 25000 == 0 ) {delay(50); printf("L1 wait... : %d\n", (i/100000));}
				if ( i >= BUSY_T )
				{
					L_1 = my_arg->now_busy = BUSY;
					printf("[[[  L_1 = BUSY!  ]]]\n");
					printf("my_arg->now_busy : %d\n", my_arg->now_busy);
					printf("sending data : %d, %d\n", my_arg->pin, my_arg->now_busy);
					i = 0 ;
					sendto(sock, my_arg, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr, sizeof(server_addr));
					delay(500);
				}
			}
			if ( BUTTON == LEFT_BUTTON2 )
			{ 
				i++;
				if ( i % 25000 == 0 ) {delay(50); printf("L2 wait... : %d\n", (i/100000));}
				if ( i >= BUSY_T )
				{
					L_2 = my_arg->now_busy = BUSY;
					printf("[[[  L_2 = BUSY!  ]]]\n");
					printf("my_arg->now_busy : %d\n", my_arg->now_busy);
					printf("sending data : %d, %d\n", my_arg->pin, my_arg->now_busy);
					i = 0 ;
					sendto(sock, my_arg, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr, sizeof(server_addr));
					delay(500);
				}
			}
		}
		else{
			i = 0;
		}	
	}
}

int main()
{
	int sock;
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) <0){
		perror("socket ");
		return 1;
	}
  	// Setting 
	memset(&addr, 0x00, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("172.20.10.8");
	addr.sin_port = htons(PORT);
	printf("waiting for messages\n");
	addr_len = sizeof(client_addr);

	wiringPiSetup();
	delay(1000);
	//set_BUTTON 
	pinMode(DOWN_BUTTON1, INPUT);
	pinMode(DOWN_BUTTON2, INPUT);
	pinMode(RIGHT_BUTTON1, INPUT);
	pinMode(RIGHT_BUTTON2, INPUT);
	printf("!!strat!!\ninitialize PIN...\n");
	//set LED
	light(DOWN_RED, DOWN_BLUE, DOWN_GREEN, RIGHT_RED, 222);
	light(RIGHT_BLUE, RIGHT_GREEN, UP_RED, UP_BLUE, 222);
	light(UP_GREEN, LEFT_RED, LEFT_BLUE, LEFT_GREEN, 222);
	delay(2000);
	pthread_t D1_thd, D2_thd, R1_thd, R2_thd, U1_thd, U2_thd, L1_thd, L2_thd;
	pthread_t traffic_thd;
	send_buff my_node[8]; 
	my_node[0].pin = DOWN_BUTTON1;
	my_node[1].pin = DOWN_BUTTON2;
	my_node[2].pin = RIGHT_BUTTON1;
	my_node[3].pin = RIGHT_BUTTON2;
	my_node[4].pin = UP_BUTTON1;
	my_node[5].pin = UP_BUTTON2;
	my_node[6].pin = LEFT_BUTTON1;
	my_node[7].pin = LEFT_BUTTON2;
 
    //sendto(sock, &rcv_buffer, sizeof(send_buff)+1, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
	printf("-----------\n");
	int i = 0;
	for(i=0 ;i< 8;i++)
	{
		my_node[i].sock = sock;	// my node per button
	}
	send_buff hi_node;
	hi_node.pin = 369, hi_node.now_busy = 123;

	 pthread_create(&D1_thd, NULL, busy_check, &my_node[0]);
	 pthread_create(&D2_thd, NULL, busy_check, &my_node[1]);
	 pthread_create(&R1_thd, NULL, busy_check, &my_node[2]);
	 pthread_create(&R2_thd, NULL, busy_check, &my_node[3]);
	 pthread_create(&U1_thd, NULL, busy_check, &my_node[4]);
	 pthread_create(&U2_thd, NULL, busy_check, &my_node[5]);
	 pthread_create(&L1_thd, NULL, busy_check, &my_node[6]);
	 pthread_create(&L2_thd, NULL, busy_check, &my_node[7]);
	 pthread_create(&traffic_thd, NULL, traffic_light, (void*)DOWN_BUTTON1); 
	 addr_len = sizeof(server_addr);
	 if(sendto(sock, &hi_node, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr,  addr_len) < 0 )
	 {
		 printf(" it didn't work \n");
	 }
	 send_buff rcv_buffer;
	 while(1)
	 {
		 printf(" Wating for recv \n" ) ;
		 if(recvfrom(sock, &rcv_buffer, sizeof(send_buff)+1, 0, (struct sockaddr *)&addr, &addr_len) < 0)
			 printf("ERROR IN RECV! ");
		 printf("ip : %s\n", inet_ntoa(addr.sin_addr));
		 printf("received data : %d, %d\n", rcv_buffer.pin, rcv_buffer.now_busy);
		 if(rcv_buffer.now_busy == 1){
			 what_node_busy[rcv_buffer.pin-1] = BUSY;
		 }	// busy
		 else{
			 what_node_busy[rcv_buffer.pin-1] = NOT_BUSY;
		 }	// not_busy 
     }

	 

	 pthread_join(D1_thd, NULL);
	 pthread_join(D2_thd, NULL);
	 pthread_join(R1_thd, NULL);
	 pthread_join(R2_thd, NULL);
	 pthread_join(U1_thd, NULL);
	 pthread_join(U2_thd, NULL);
	 pthread_join(L1_thd, NULL);
	 pthread_join(L2_thd, NULL);
	 pthread_join(traffic_thd, NULL);
	 close(sock);
	 return 0;

}