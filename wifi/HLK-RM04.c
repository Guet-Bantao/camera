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
	
	/*�����Ƿ�Ϊ�ն��豸 */  
  	if(0 == isatty(STDIN_FILENO))
  	{
        printf("standard input is not a terminal device/n");
		return -1;
  	}

	set_uart(fd,115200,0,8,1,'N');
	
	/* ��uart_read����ʹ�����в����κ�fd*/
	FD_ZERO(&uart_read);  
	/* ��fd����uart_read���� */
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
	
	/*�õ���fdָ��������ز�����������*/
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
		
	/*�޸Ŀ���ģʽ����֤���򲻻�ռ�ô���*/  
    opt.c_cflag |= CLOCAL;  
    /*�޸Ŀ���ģʽ��ʹ���ܹ��Ӵ����ж�ȡ�������� */ 
    opt.c_cflag |= CREAD;

	/*�������������� */ 
	switch(flow_ctrl)  
	{  
   	case 0 ://��ʹ��������  
          	opt.c_cflag &= ~CRTSCTS;  
          	break;     
   	case 1 ://ʹ��Ӳ��������  
          	opt.c_cflag |= CRTSCTS;  
          	break;  
   	case 2 ://ʹ�����������  
          	opt.c_cflag |= IXON | IXOFF | IXANY;  
          	break;  
	default:     
            printf("Unsupported mode flow\n");  
            return -1;
	}

	/*����������־λ  */
    opt.c_cflag &= ~CSIZE;

	/*��������λ*/
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

	/*����У��λ */ 
   	 switch (parity)  
	{    
   	case 'n':  
   	case 'N': //����żУ��λ��  
             	opt.c_cflag &= ~PARENB;   
             	opt.c_iflag &= ~INPCK;      
             	break;   
   	case 'o':    
   	case 'O'://����Ϊ��У��      
            	 opt.c_cflag |= (PARODD | PARENB);   
            	 opt.c_iflag |= INPCK;               
             	break;   
   	case 'e':   
   	case 'E'://����ΪżУ��    
           	 opt.c_cflag |= PARENB;         
            	 opt.c_cflag &= ~PARODD;         
            	 opt.c_iflag |= INPCK;        
            	 break;  
  	 	case 's':  
   	case 'S': //����Ϊ�ո�   
             	opt.c_cflag &= ~PARENB;  
             	opt.c_cflag &= ~CSTOPB;  
             	break;   
    	default:    
             	printf("Unsupported parity\n");      
             	return -1;   
	 }

	 /* ����ֹͣλ */  
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

	/*����ʹ���������ʱû���յ��س�����Ҳ�ܷ���*/
	//opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	opt.c_oflag &= ~OPOST;//ԭʼ�������
	/*��������,����0x0a��0x0d,�س��ͻ��в���ͬһ���ַ�*/
    	opt.c_oflag &= ~(ONLCR | OCRNL); 

	opt.c_iflag &= ~(ICRNL | INLCR);
	/*ʹ��ASCII��׼��XON��XOFF�ַ��ܴ���*/
    	opt.c_iflag &= ~(IXON | IXOFF | IXANY);    //����Ҫ�������ַ�

	/*�����������������������ݣ����ǲ��ٶ�ȡ*/
	tcflush(fd, TCIFLUSH);
	
	/*��������ݿ��ã���read��෵����Ҫ����ֽ�������������ݿ��ã���read��������0*/
    	opt.c_cc[VTIME] = 0;        //���ó�ʱ
    	opt.c_cc[VMIN] = 0;        //Update the Opt and do it now

	/*
	*ʹ������
	*TCSANOW������ִ�ж����ȴ����ݷ��ͻ��߽������
	*TCSADRAIN���ȴ��������ݴ�����ɺ�ִ��
	*TCSAFLUSH����������buffers  �ı�ʱִ��
	*/
	if (tcsetattr(fd,TCSANOW,&opt) != 0)
	{
		printf("uart set error!\n");
		return -1; 
	}
	return 0;
	
}


	
