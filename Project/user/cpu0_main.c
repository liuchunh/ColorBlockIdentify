/*********************************************************************************************************************
* TC264 Opensourec Library 即（TC264 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是 TC264 开源库的一部分
*
* TC264 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          cpu0_main
* 公司名称          成都逐飞科技有限公司
* 版本信息          查看 libraries/doc 文件夹内 version 文件 版本说明
* 开发环境          ADS v1.9.20
* 适用平台          TC264D
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者                备注
* 2022-09-15       pudding            first version
********************************************************************************************************************/
#include "zf_common_headfile.h"
#include "CCL.h"
#include "zf_driver/zf_driver_uart.h"
#include "code/ComOutput.h"
#pragma section all "cpu0_dsram"
// 将本语句与#pragma section all restore语句之间的全局变量都放在CPU0的RAM中
// *************************** 例程硬件连接说明 ***************************
// 接入总钻风灰度数字摄像头 对应主板摄像头接口 请注意线序
//      模块管脚            单片机管脚
//      TXD                 查看 zf_device_mt9v03x.h 中 MT9V03X_COF_UART_TX 宏定义
//      RXD                 查看 zf_device_mt9v03x.h 中 MT9V03X_COF_UART_RX 宏定义
//      PCLK                查看 zf_device_mt9v03x.h 中 MT9V03X_PCLK_PIN 宏定义
//      VSY                 查看 zf_device_mt9v03x.h 中 MT9V03X_VSYNC_PIN 宏定义
//      D0-D7               查看 zf_device_mt9v03x.h 中 MT9V03X_DATA_PIN 宏定义 从该定义开始的连续八个引脚
//      GND                 核心板电源地 GND
//      3V3                 核心板 3V3 电源
// 接入2寸IPS模块
// *************************** 例程硬件连接说明 ***************************
//      模块管脚            单片机管脚
//      BL                  查看 zf_device_ips200_parallel8.h 中 IPS200_BL_PIN 宏定义  默认 P15_3
//      CS                  查看 zf_device_ips200_parallel8.h 中 IPS200_CS_PIN 宏定义  默认 P15_5
//      RST                 查看 zf_device_ips200_parallel8.h 中 IPS200_RST_PIN 宏定义 默认 P15_1
//      RS                  查看 zf_device_ips200_parallel8.h 中 IPS200_RS_PIN 宏定义  默认 P15_0
//      WR                  查看 zf_device_ips200_parallel8.h 中 IPS200_WR_PIN 宏定义  默认 P15_2
//      RD                  查看 zf_device_ips200_parallel8.h 中 IPS200_RD_PIN 宏定义  默认 P15_4
//      D0-D7               查看 zf_device_ips200_parallel8.h 中 IPS200_Dx_PIN 宏定义  默认 P11_9/P11_10/P11_11/P11_12/P13_0/P13_1/P13_2/P13_3
//      GND                 核心板电源地 GND
//      3V3                 核心板 3V3 电源



// *************************** 例程测试说明 ***************************
// 1.核心板烧录完成本例程 将核心板插在主板上 插到底
// 2.摄像头接在主板的摄像头接口 注意线序2寸IPS模块插入主板屏幕接口
// 3.主板上电 或者核心板链接完毕后上电 核心板按下复位按键
// 4.屏幕会显示初始化信息然后显示摄像头图像
// 如果发现现象与说明严重不符 请参照本文件最下方 例程常见问题说明 进行排查
// **************************** 代码区域 ****************************

// 单排排针 SPI 两寸屏 这里宏定义填写 IPS200_TYPE_SPI
// 双排排针 并口两寸屏 这里宏定义填写 IPS200_TYPE_PARALLEL8
#define IPS200_TYPE     (IPS200_TYPE_SPI) 

Vec3b hsvImage[MaskHeight][MaskWidth];
unsigned char mask[MaskHeight][MaskWidth];

Run run[MaskHeight][MaxRunsPerRow];
uint8_t RunsCount[MaskHeight];
Component component[MaxRunsPerRow * MaskHeight];

unsigned char LeftTopX, LeftTopY, RightBottomX, RightBottomY;

const Vec3b Lower = {26, 43, 46}; // HSV 范围下限
const Vec3b Upper = {34, 255, 255}; // HSV 范
const Vec3b PixelEps = {10, 5, 5}; // HSV 范围容差

int core0_main(void){
    clock_init();                   // 获取时钟频率<务必保留>
    debug_init();                   // 初始化默认调试串口
    // 此处编写用户代码 例如外设初始化代码等

    ips200_init(IPS200_TYPE);
    ips200_show_string(0, 0, "scc8660 init.");

    while (TRUE){
        if (scc8660_init())
            ips200_show_string(0, 80, "scc8660 reinit.");
        else
            break;
        system_delay_ms(500);                                                   // 短延时快速闪灯表示异常
    }
    ips200_show_string(0, 16, "init success.");

    // 此处编写用户代码 例如外设初始化代码等
    cpu_wait_event_ready();         // 等待所有核心初始化完毕
    while (TRUE){
        // 此处编写需要循环执行的代码
        if (scc8660_finish_flag){
            ips200_displayimage8660(scc8660_image[0], SCC8660_W, SCC8660_H);                     // 显示原始图像

            LeftTopX = LeftTopY = RightBottomX = RightBottomY = 0;
            memset(mask, 0, sizeof(mask));
            memset(component, 0, sizeof(component));
            memset(RunsCount, 0, sizeof(RunsCount));
            memset(run, 0, sizeof(run));

            ConvertImg2HSV(scc8660_image);
            ImgInHsvRange(hsvImage, Lower, Upper, PixelEps);

            uint16_t RunsCountRow = 0;
            for (int y = 0; y < MaskHeight; y ++ ){
                if (ExtractRunsFromRow(mask[y], MaskWidth, run[y], MaxRunsPerRow, &RunsCountRow)){
                    RunsCount[y] = RunsCountRow;
                }
                else{
                    // ips200_show_string(0, 32, "ExtractRunsFromRow failed.");
                    uart_write_string(UART_0, "ExtractRunsFromRow failed.\r\n");
                }
            }

            uint16_t Label = MergeRunsToComponent(run, component, MaskHeight, RunsCount, MaxRunsPerRow * MaskHeight);
            size_t MaxArea = 0;

            // 在合并后, 仅有根结点且 used 标志为 true 的连通块才算为有效连通块
            for (int i = 1; i <= Label; i ++ ){
                if (component[i].Used && component[i].area > MaxArea && component[i].parent == i){
                    MaxArea = component[i].area;

                    LeftTopX = component[i].minX;
                    LeftTopY = component[i].minY;
                    RightBottomX = component[i].maxX;
                    RightBottomY = component[i].maxY;
                }
            }

            if (LeftTopX == 0 && LeftTopY == 0 && RightBottomX == 0 && RightBottomY == 0){
                uart_write_string(UART_0, "No target found.\r\n");
            }
            else{
                uart_write_string(UART_0, "Target at [");
                uart_write_integer(UART_0, LeftTopX);
                uart_write_string(UART_0, ", ");
                uart_write_integer(UART_0, LeftTopY);
                uart_write_string(UART_0, "] -> [");
                uart_write_integer(UART_0, RightBottomX);
                uart_write_string(UART_0, ", ");
                uart_write_integer(UART_0, RightBottomY);
                uart_write_string(UART_0, "]\r\n");
            }

            scc8660_finish_flag = 0;
        }

        
        // 此处编写需要循环执行的代码
    }
}

#pragma section all restore
// **************************** 代码区域 ****************************
// *************************** 例程常见问题说明 ***************************
// 遇到问题时请按照以下问题检查列表检查
// 问题1：屏幕不显示
//      如果使用主板测试，主板必须要用电池供电 检查屏幕供电引脚电压
//      检查屏幕是不是插错位置了 检查引脚对应关系
//      如果对应引脚都正确 检查一下是否有引脚波形不对 需要有示波器
//      无法完成波形测试则复制一个GPIO例程将屏幕所有IO初始化为GPIO翻转电平 看看是否受控
// 问题2：显示 reinit 字样
//      检查接线是否正常
//      主板供电是否使用电量充足的电池供电
// 问题2：显示图像杂乱 错位
//      检查摄像头信号线是否有松动
