#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <wifi_manager.h>
#include <camera_manager.h>
#include <lcd_manager.h>
#include <convert_manager.h>
#include <merge.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>



int main(int argc, char **argv)
{
	int error;
	lcd_device_opr lcd_device;
	camera_device_opr camera_device;
	video_buf frame_buf;
	int pixel_format_camera;//in 像素格式
    int pixel_format_lcd;//out
    convert_opr* convert;

	int lcd_width;
    int lcd_heigt;
    int lcd_bpp;
	int top_left_x;
    int top_left_y;

	video_buf camera_buf;
	video_buf convert_buf;
	//video_buf zoom_buf;
	video_buf* camera_buf_cur;
	
	//wifi_opr  *wifi;

	if (argc != 2)
    {
        printf("Usage:\n");
        printf("%s </dev/video0,1,...>\n", argv[0]);
        return -1;
    }

	/* register lcd devices */
	lcd_init();
	/* choose the lcd device */
	lcd_device_init("/dev/fb0", &lcd_device);
	get_lcd_resolution(&lcd_width, &lcd_heigt, &lcd_bpp);
	get_lcd_buf_display(&frame_buf);
	pixel_format_lcd = frame_buf.pixel_format;

	/* register camera devices */
	camera_init();
	error = camera_device_init(argv[1], &camera_device);
	if (error)
    {
        printf("VideoDeviceInit for %s error!\n", argv[1]);
        return -1;
    }
	pixel_format_camera = camera_device.opr->get_fmat(&camera_device);

	convert_init();
	convert = get_convert_formats(pixel_format_camera, pixel_format_lcd);
    if (NULL == convert)
    {
        printf("can not support this format convert\n");
        return -1;
    }
	printf("the formats is:%s\n",convert->name);

	/* 启动摄像头设备 */
    error = camera_device.opr->start(&camera_device);
    if (error)
    {
        printf("StartDevice for %s error!\n", argv[1]);
        return -1;
    }

	memset(&camera_buf, 0, sizeof(camera_buf));
	memset(&convert_buf, 0, sizeof(convert_buf));
	convert_buf.pixel_format			= pixel_format_lcd;
	convert_buf.video_pixel_datas.bpp	= lcd_bpp;
	//memset(&zoom_buf, 0, sizeof(zoom_buf));

	while (1)
    {
		/* 读入摄像头数据 */
        error = camera_device.opr->get_frame(&camera_device, &camera_buf);
		if (error)
        {
            printf("GetFrame for %s error!\n", argv[1]);
            return -1;
        }
		camera_buf_cur = &camera_buf;

		if (pixel_format_camera != pixel_format_lcd)
        {
            /* 转换为RGB */
            error = convert->convert(&camera_buf, &convert_buf);
            //printf("Convert %s, ret = %d\n", convert->name, error);
            if (error)
            {
                printf("Convert for %s error!\n", argv[1]);
                //return -1;
                //lcd_device.opr->show_page(&frame_buf.video_pixel_datas, &lcd_device);
                error = camera_device.opr->put_frame(&camera_device, &camera_buf);
                continue;
            }            
            camera_buf_cur = &convert_buf;
        }

		/* 合并进framebuffer */
        /* 接着算出居中显示时左上角坐标 */
        top_left_x = (lcd_width - camera_buf_cur->video_pixel_datas.width) / 2;
        top_left_y = (lcd_heigt - camera_buf_cur->video_pixel_datas.height) / 2;

		picture_merge(top_left_x, top_left_y, &camera_buf_cur->video_pixel_datas, &frame_buf.video_pixel_datas);

		/* 把framebuffer的数据刷到LCD上, 显示 */
        lcd_device.opr->show_page(&frame_buf.video_pixel_datas, &lcd_device);
		error = camera_device.opr->put_frame(&camera_device, &camera_buf);
        if (error)
        {
            printf("PutFrame for %s error!\n", argv[1]);
            return -1;
        } 
	}
	
	return 0;
}













