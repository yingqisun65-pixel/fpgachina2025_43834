## bd/create_bd.tcl 
从 0 重建 Block Design（包含 IP 参数、连线、地址映射、画布布局）。
适合：拉起一模一样的 BD，而不依赖已保存的 .xpr 工程。

###怎么用（Vivado GUI / Tcl Console）

-打开 Vivado，新建一个空工程并选好器件（例：xc7z020clg400-1）。

-在 Tcl Console 执行：
```
source ./vivado/bd/create_bd.tcl     ;# 重建 BD（默认名 dection）
make_wrapper -files [get_files *.bd] -top
add_files [glob ./.srcs/sources_1/bd/*/hdl/*_wrapper.v]
update_compile_order -fileset sources_1
```

>（如有 XDC）自己 add 到 constrs_1。  
>提示：不同 Vivado 版本导出 create_bd.tcl 的方式略有差异；此脚本已能直接 source 使用。

## scripts/build_bit.tcl 
一键生成 bit/hwh/xsa：
自动创建临时工程 → 调用 create_bd.tcl 重建 BD → 生成 wrapper → 综合/实现 → 输出 .bit/.hwh/.xsa 到 build/。

### 怎么用（Batch 一键跑）

- Linux
`vivado -mode batch -source vivado/scripts/build_bit.tcl`

- Windows（PowerShell）
`vivado -mode batch -source vivado\scripts\build_bit.tcl`


## 常用可改变量（在 build_bit.tcl 顶部）：

- PART：器件（默认示例 xc7z020clg400-1）
- BD_TCL：./vivado/bd/create_bd.tcl
- OUT_DIR：输出目录（默认 ./build）
- IP_REPO / XDC：如有本地 IP 仓库或约束文件可填路径
- 运行完成后在 build/ 下可得到：
```
<project>.bit
<bd_name>.hwh
<project>.xsa   # (含 bit)
```

