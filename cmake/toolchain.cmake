set(PREFIX "$ENV{HOME}/MRS_Toolchain_Linux_X64_V240/Toolchain/RISC-V Embedded GCC15/bin/riscv32-wch-elf-")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

set(CMAKE_C_COMPILER "${PREFIX}gcc")
set(CMAKE_ASM_COMPILER "${PREFIX}gcc")
set(CMAKE_OBJCOPY "${PREFIX}objcopy")
set(CMAKE_SIZE "${PREFIX}size")
