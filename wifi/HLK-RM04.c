#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <unistd.h>

#include <wifi_manager.h>


int fd;
fd_set uart_read;
//static pthread_t recv_tread_id;

int set_uart(int fd,int boud,int flow_ctrl,int databits,int stopbits,int parity);

static int rm04_init(char * name)//"/dev/ttySAC1"
{
	fd = open(name, O_RDWR | O_NOCTTY);
	if (fd < 0)
	{
		printf("can't open /dev/ttySAC1\n");
		return -1;
	}
	
	/*测试是否为终端设备 */  
  	if(0 == isatty(STDIN_FILENO))
  	{
        printf("standard input is not a terminal device/n");
		return -1;
  	}

	set_uart(fd,115200,0,8,1,'N');
	
	/* 将uart_read清零使集合中不含任何fd*/
	FD_ZERO(&uart_read);  
	/* 将fd加入uart_read集合 */
    FD_SET(fd,&uart_read);
	
	return 0;
}

int rm04_send_safe(char * send_str,char * mach_str)
{
	int read_flag=0,send_num=0;
	char rcv_buf[64];
	fd_set uart_read;
	struct timeval time;

	time.tv_sec = 1;
    time.tv_usec = 0; 
	do
	{
		send_num++;
		write(fd, send_str, strlen(send_str));
		read_flag = select(fd+1,&uart_read,NULL,NULL,&time);
		memset(rcv_buf,0,sizeof(rcv_buf)/sizeof(char));
		if(read_flag)  
       	{ 
            read(fd,rcv_buf,sizeof(rcv_buf)/sizeof(char)); 
			printf("data = %s\n",rcv_buf); 
		}
		if(send_num>50)
			return -1;
	}
	while(!(strstr(rcv_buf,mach_str)!=0));
	   	
	return 0;
}

int rm04_send(char * send_str)
{
	write(fd, send_str, strlen(send_str));
	return 0;
}


static wifi_opr hlk_rm04_device_opr = {
	.name		= "hlk-rm04",
	.init		= rm04_init,
	.send_safe	= rm04_send_safe,
	.send		= rm04_send,
};

int rm04_register(void)
{
	return register_wifi_opr(&hlk_rm04_device_opr);
}


int set_uart(int fd,int boud,int flow_ctrl,int databits,int stopbits,int parity)  
{ 
	struct termios opt;
	int i;
	int baud_rates[] = { B9600, B19200, B57600, B115200, B38400 };
	int speed_rates[] = {9600, 19200, 57600, 115200, 38400};
	
	/*得到与fd指向对象的相关参数，并保存*/
	if (tcgetattr(fd, &opt) != 0)
	{
		printf("fail get fd data \n");
		return -1;
	}
	for ( i= 0;  i < sizeof(speed_rates) / sizeof(int);  i++)  
	{  
		if (boud == speed_rates[i])  
		{               
			cfsetispeed(&opt, baud_rates[i]);   
			cfsetospeed(&opt, baud_rates[i]);    
		}  
	} 
		
	/*修改控制模式，保证程序不会占用串口*/  
    opt.c_cflag |= CLOCAL;  
    /*修改控制模式，使得能够从串口中读取输入数据 */ 
    opt.c_cflag |= CREAD;

	/*设置数据流控制 */ 
	switch(flow_ctrl)  
	{  
   	case 0 ://不使用流控制  
          	opt.c_cflag &= ~CRTSCTS;  
          	break;     
   	case 1 ://使用硬件流控制  
          	opt.c_cflag |= CRTSCTS;  
          	break;  
   	case 2 ://使用软件流控制  
          	opt.c_cflag |= IXON | IXOFF | IXANY;  
          	break;  
	default:     
            printf("Unsupported mode flow\n");  
            return -1;
	}

	/*屏蔽其他标志位  */
    opt.c_cflag &= ~CSIZE;

	/*设置数据位*/
	switch (databits)  
	{    
   	case 5    :  
                 opt.c_cflag |= CS5;  
                 break;  
   	case 6    :  
                 opt.c_cflag |= CS6;  
                 break;  
   	case 7    :      
             	opt.c_cflag |= CS7;  
             	break;  
   	case 8:      
             	opt.c_cflag |= CS8;  
             	break;    
   	default:     
             	printf("Unsupported data size\n");  
             	return -1;   
    	} 

	/*设置校验位 */ 
   	 switch (parity)  
	{    
   	case 'n':  
   	case 'N': //无奇偶校验位。  
             	opt.c_cflag &= ~PARENB;   
             	opt.c_iflag &= ~INPCK;      
             	break;   
   	case 'o':    
   	case 'O'://设置为奇校验      
            	 opt.c_cflag |= (PARODD | PARENB);   
            	 opt.c_iflag |= INPCK;               
             	break;   
   	case 'e':   
   	case 'E'://设置为偶校验    
           	 opt.c_cflag |= PARENB;         
            	 opt.c_cflag &= ~PARODD;         
            	 opt.c_iflag |= INPCK;        
            	 break;  
  	 	case 's':  
   	case 'S': //设置为空格   
             	opt.c_cflag &= ~PARENB;  
             	opt.c_cflag &= ~CSTOPB;  
             	break;   
    	default:    
             	printf("Unsupported parity\n");      
             	return -1;   
	 }

	 /* 设置停止位 */  
	switch (stopbits)  
	{    
   	case 1:     
             	opt.c_cflag &= ~CSTOPB;
		break;   
   	case 2:     
             	opt.c_cflag |= CSTOPB; 
		break;  
   	default:     
                 printf("Unsupported stop bits\n");   
                   return -1;  
	}

	/*设置使得输入输出时没接收到回车或换行也能发送*/
	//opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	opt.c_oflag &= ~OPOST;//原始数据输出
	/*下面两句,区分0x0a和0x0d,回车和换行不是同一个字符*/
    	opt.c_oflag &= ~(ONLCR | OCRNL); 

	opt.c_iflag &= ~(ICRNL | INLCR);
	/*使得ASCII标准的XON和XOFF字符能传输*/
    	opt.c_iflag &= ~(IXON | IXOFF | IXANY);    //不需要这两个字符

	/*如果发生数据溢出，接收数据，但是不再读取*/
	tcflush(fd, TCIFLUSH);
	
	/*如果有数据可用，则read最多返回所要求的字节数。如果无数据可用，则read立即返回0*/
    	opt.c_cc[VTIME] = 0;        //设置超时
    	opt.c_cc[VMIN] = 0;        //Update the Opt and do it now

	/*
	*使能配置
	*TCSANOW：立即执行而不等待数据发送或者接受完成
	*TCSADRAIN：等待所有数据传递完成后执行
	*TCSAFLUSH：输入和输出buffers  改变时执行
	*/
	if (tcsetattr(fd,TCSANOW,&opt) != 0)
	{
		printf("uart set error!\n");
		return -1; 
	}
	return 0;
	
}


	
