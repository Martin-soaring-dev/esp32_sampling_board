# ESP32 传感采样板 — 用户手册

**概述**
- **项目**: ESP32 传感采样板固件。
- **用途**: 读取连接传感器的数据，通过串口与上位机交互，支持命令式控制与查询。

**功能清单**
- **传感采样**: 按配置周期采集传感器数据或按命令触发采样。
- **命令解析与分发**: 支持 I01/I02/I03/I114/I999 等命令；实现模块化命令处理。
- **串口通信**: 支持 ASCII 文本与可选的二进制帧协议。

**硬件说明**
- 芯片: ESP32（具体板型与引脚请参考硬件资料）。
- 采样引脚、外设与电源接法请参考硬件原理图（仓库中若有，请补充）。

**软件架构与主要源码文件**
- **入口**: [src/main.cpp](src/main.cpp)
- **协议解析**: [src/protocol_decoder.cpp](src/protocol_decoder.cpp) / [src/protocol_decoder.h](src/protocol_decoder.h)
- **命令分发**: [src/command/dispatcher.cpp](src/command/dispatcher.cpp) / [src/command/dispatcher.h](src/command/dispatcher.h)
- **命令模块**: 位于 [src/command/](src/command/) 下的 `i01`, `i02`, `i03`, `i114`, `i999` 等文件（参见各自的 .cpp/.h）
- **常量/类型**: [src/command/types.h](src/command/types.h)
- **配置**: [src/configuration.h](src/configuration.h)

**上位机通讯协议**

目标: 定义上位机（PC）与 ESP32 之间的双向消息格式，支持 ASCII 调试模式与可选二进制帧模式。

- 传输层: 默认 UART（USB 串口），建议波特率 `115200`，配置 8N1，需与 `configuration.h` 保持一致。

两种模式说明:

1) ASCII 行文本模式（便于调试）
- 格式: <CMD>[,param1,param2,...]\n
- 示例请求: `I01,D12,S100\n`
- 示例响应: `OK,I01,STARTED,D12,S100\n` 或 `ERR,I01,INVALID_PARAM\n`

2) 二进制帧（高效，建议在稳定后使用）
- 建议帧结构（示例）:
  - Start: `0xAA`
  - Ver: 1 byte
  - CmdID: 1 byte
  - Len: 1 byte (payload 长度)
  - Payload: N bytes
  - Checksum: 1 byte (XOR 或 CRC-8)
  - End: `0x55`（可选）
- 示例（十六进制）: `AA 01 01 03 0C 64 00 B4 55`（含解释见下文）

校验建议: 简单场景可选 XOR（对 Ver/ CmdID/ Len/ Payload），更高可靠性请用 CRC-8（多项式 0x07）。

错误与状态:
- ASCII: 使用 `OK` / `ERR` 前缀并附加描述。
- 二进制: 状态字节 `0x00` 表示成功，非零为错误码（详见 `types.h` 或本手册错误码表）。

解析伪代码:
```
state = WAIT_START
while read byte b:
  if state == WAIT_START and b == 0xAA: state = READ_HDR
  elif state == READ_HDR: read ver, cmd, len; read payload; read chk; optional read end
    if verify_checksum(...): dispatch(cmd, payload)
    else: send_error(ERR_CHECKSUM)
    state = WAIT_START
```

**命令定义（上位机与 ESP 交互）**

说明: 下列命令同时给出 ASCII 文本格式与二进制帧示例（CmdID 数值请以 `src/command/types.h` 为准；下列 CmdID 为说明性示例）。

1) I01 — 设置传感器参数并开始采集
- 作用: 指定采样引脚与采样频率，启动连续采集。
- ASCII 请求: `I01,D<pin>,S<freq>\n` 例如 `I01,D12,S100\n`（freq 单位为 ms）
- ASCII 成功响应: `OK,I01,STARTED,D12,S100\n`
- ASCII 错误: `ERR,I01,INVALID_PARAM\n`
- 二进制（示例）:
  - CmdID: `0x01`
  - Payload: [pin:uint8][freq:uint16 Little-endian(ms)]
  - 上位机->ESP: `AA 01 01 03 <pin> <freq_L> <freq_H> <CHK> 55`
  - ESP 成功响应: 返回 CmdID|0x80（0x81）并带状态/确认 payload。

2) I02 — 重置采集
- 作用: 将采集子系统重置至初始状态（清除缓存/计数等）。
- ASCII 请求: `I02,RESET\n` 或 `I02\n`
- ASCII 响应: `OK,I02,RESET\n` 或 `ERR,I02,NOT_RUNNING\n`
- 二进制: CmdID: `0x02`，Payload 可为空（len=0），ESP 返回状态字节。

3) I03 — 停止采集
- 作用: 停止当前的连续采集动作。
- ASCII 请求: `I03,STOP\n` 或 `I03\n`
- ASCII 响应: `OK,I03,STOPPED\n`
- 二进制: CmdID: `0x03`，Payload 空，返回状态码表示成功/失败。

4) I114 — 查询系统状态
- 作用: 返回系统信息（串口设置、当前采样引脚与频率、运行时间等）。
- ASCII 请求: `I114,STATUS\n` 或 `I114\n`
- ASCII 响应（示例，CSV 风格）: `OK,I114,BAUD,115200,PIN,D:12,FREQ:100,UPTIME:12345\n`
- 二进制: CmdID 示例 `0x72`（请以 `types.h` 为准），ESP 返回结构化 payload，例如 `[baud:uint32][pin:uint8][freq:uint16][uptime:uint32]`（须在文档中约定字节序）。

5) I999 — 重启系统
- 作用: 立即或延迟重启设备。
- ASCII 请求: `I999,REBOOT\n`
- ASCII 响应: `OK,I999,REBOOTING\n`（随后设备会重启）
- 二进制: CmdID 示例 `0xE7`，Payload 可为空，ESP 返回确认后执行重启。

错误码示例（建议统一定义于 `src/command/types.h`）:
- `0x00` 成功
- `0x01` 参数错误
- `0x02` 未知命令
- `0x03` 校验失败
- `0x04` 硬件错误

**示例：ASCII 与二进制交互**
- ASCII: 发 `I01,D12,S100` → ESP 返回 `OK,I01,STARTED,D12,S100`
- 二进制: 发 `AA 01 01 03 0C 64 00 <CHK> 55`（pin=12, freq=100ms）→ ESP 返回 `AA 01 81 01 00 <CHK> 55`（示意成功响应）

**构建与上传（PlatformIO）**
- 构建: 
```bash
pio run
```
- 上传:
```bash
pio run --target upload
```
- 串口监视:
```bash
pio device monitor
```

**使用与调试建议**
- 开发阶段优先使用 ASCII 模式便于手工调试。
- 稳定后启用二进制模式以节省带宽并提高解析效率。
- 将 `CmdID` 与 `src/command/types.h` 保持同步；明确约定字节序（建议 little-endian）与校验算法（XOR 或 CRC-8）。

**故障排查**
- 无响应: 检查串口配置（波特率、端口）是否与 `configuration.h` 一致。
- 命令未识别: 确认命令拼写与大小写，或用二进制帧检查校验是否一致。
- 重启或崩溃: 观察串口启动日志，检查内存使用与任务堆栈。

**后续工作建议**
- 将每个命令的确切 CmdID 与 payload 格式写入 `src/command/types.h` 并同步本手册。
- 提供上位机解析库（最小 SDK），实现 `build_frame()` / `parse_frame()` 与校验函数，方便用户集成。

---
此文档位于仓库 `docs/USER_MANUAL.md`，如需我把内容进一步细化为英文版或按函数级别生成 API 文档，请告知。
