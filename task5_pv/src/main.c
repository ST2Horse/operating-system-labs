#include "pv_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(){
	srand((unsigned int)time(NULL));

	while(1){
		print_line();
		printf("信号量与P/V操作系统实验\n");
		print_line();
		printf("1. 生产者 / 消费者问题\n");
        	printf("2. 写者优先读者 / 写者问题\n");
        	printf("3. 哲学家就餐问题\n");
        	printf("4. 吸烟者问题\n");
        	printf("0. 退出\n");
		print_line();

		int choice = read_int("请选择实验模块：",0,4);

		switch(choice){
			case 1:
				run_producer_consumer();
				break;
			case 2:
				run_reader_writer();
				break;
			case 3:
                		run_dining_philosophers();
                		break;
            		case 4:
                		run_smokers();
                		break;
            		case 0:
                		printf("程序结束。\n");
                		return 0;
            		default:
                		break;
			}
	}
}
