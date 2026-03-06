# esp32_sampling_board

**原文连接**：[Martin-soaring-dev/esp32_sampling_board](https://github.com/Martin-soaring-dev/esp32_sampling_board)

## 1. Get start

### (1) 工具准备

- 可以从这里[^1]获取ESP32的下载工具。在/frimware/tool路径下也添加了该工具。
- 安装上位机<**SensorSamplingBoard_web.exe**>。

### (2) 下载firmware

相关配置参考了[^2]的经验。

打开工具后，按照下图进行选择操作：

![image-20260306210502088](assets/image-20260306210502088.png)

然后点击OK进入图形界面：

![image-20260306210537411](assets/image-20260306210537411.png)

Step 2 然后按照下图选择文件：

![image-20260306210738741](assets/image-20260306210738741.png)

具体来说，按照下表进行配置即可

| 文件名         | 地址    |
| -------------- | ------- |
| bootloader.bin | 0x1000  |
| partitions.bin | 0x8000  |
| boot_app0.bin  | 0xE000  |
| firmware.bin   | 0x10000 |

SPI Flash Config 保持图中的默认设置即可

波特率选择为：460800

COM口根据实际情况选择，查看方法为，此电脑→右击→管理→设备管理器→端口→查看连接ESP32后新出现的COM口

![image-20260306211150113](assets/image-20260306211150113.png)

然后先进行**擦除**，点击""**ERASE**""，清空Flash中的数据：

![image-20260306211359752](assets/image-20260306211359752.png)

然后进行下载，点击"**START**"，开始下载数据：

![image-20260306211452864](assets/image-20260306211452864.png)

### (3)上位机连接与测试下载是否成功

打开<**SensorSamplingBoard**>

![image-20260306211615125](assets/image-20260306211615125.png)

选择合适的COM和波特率115200，点击连接，连接成功且固件成功下载时，会返回如下信息，最后一条应为esp32_sampling_board_online：

![image-20260306211714136](assets/image-20260306211714136.png)

## 2. 采集功能

![image-20260306211936362](assets/image-20260306211936362.png)

#### 基本功能

- Pin: 用于采集的引脚序号，目前支持的引脚为2,4,12,13,14,25,26,27,32,33,34,35,36,39，如果输入错误会弹窗

![image-20260306212110371](assets/image-20260306212110371.png)

- Freq. 采样频率≤100Hz，不建议大于60Hz，稳定性会变差。
- R: 设置测阻电路中参考电阻的阻值。
- 复选框R，如果勾选，则在会图框显示R，并在保存数据时，增加电阻数值。
- 复选框Pull up：测阻电路有两种接法，参考电阻可能为上拉电阻或者下拉电阻，默认为下拉电阻；如果接线时参考电阻为上拉电阻，请勾选。



- Set按钮：设置完Pin和Freq后可以点击Set以进行配置，之后会不断采集pin的引脚数据并显示在Realtime的窗口中。

  - 如果频率设置的比较高，返回的数据在command window中太多，可以取消勾选show received MSG。

- Reset按钮：点击Reset按钮后，会重置时间戳为0ms。

- Stop按钮：点击后会停止采集。

- Sys info按钮：点击后显示系统信息：包括串口信息、引脚配置、引脚数值

- Reboot：重启采集板

- Clear：清空绘图区

- Set Scale，绘图区显示的时间范围

  

## 3. 记录功能

**概览**：

使用基本功能完成测试后，如果有需要可以进行记录

记录时间范围最长为24小时×60小时/分钟×60秒/分钟×60Hz=5,184,000个数据点，可以根据采样率Freq进行计算最大采样时长。填入duration。

**功能介绍**：

开始采集：然后可以点击record按钮，开始记录。开始记录后Record按钮会被按下，Set, Reset, Stop会变成灰色。

停止采集：当达到记录时长后，record按钮会弹起，Set, Reset, Stop, save, save as按钮可以使用；如果希望提前终止记录，可以点击record按钮，然后回提示用户是否取消采集，确认后即可停止采集（不会删除已记录的数据）。

![image-20260306213501919](assets/image-20260306213501919.png)

采集完成：采集完成后，数据会绘制在recorded图窗中，用户可以根据需要点击save按钮保存为txt文本，或者点击save as保存为csv或excel表格。

## 4. 高级功能（手动操作）

用户界面支持用户手动发送指令进行操作，现在把支持的指令列出：

- I01 DXX SYY: 设置端口XX以YY频率采样。

- I02: 重置时间戳

- I03: 停止采集

- I114: 查询系统状态

- I999: 重启采集板

  

## 参考

[^1]: [Flash 下载工具用户指南 - ESP32 - — ESP 测试工具 latest 文档](https://docs.espressif.com/projects/esp-test-tools/zh_CN/latest/esp32/production_stage/tools/flash_download_tool.html#id5)
[^2]: [【ESP32之旅】ESP32 PlatformIO 固件单独烧录_esp32固件-CSDN博客](https://blog.csdn.net/Argon_Ghost/article/details/139307638)

