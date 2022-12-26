#! /bin/sh
#
# doc : <https://blog.csdn.net/leixiaohua1020/article/details/44226355>
#       <https://ffmpeg.xianwaizhiyin.net>
#       <https://zhuanlan.zhihu.com/p/143195044>

BEAR_MARK=""
COMPILE_THREAD="7"
if (uname -a | grep -E "WSL2|Darwin" > /dev/null); then
   BEAR_MARK="--"
   echo "this is   WSL2 | Darwin"
fi

if command -v nproc > /dev/null; then
   COMPILE_THREAD=$(echo "$(nproc) - 1" | bc)   # 预留出一个核心
   echo "using ${COMPILE_THREAD} core to compile~"
fi

./configure --enable-debug

bear ${BEAR_MARK} make -j8 && bear --append ${BEAR_MARK} make examples -j${COMPILE_THREAD}

if [[ "$(uname -a)" == *Linux*Microsoft* ]]; then
   # WSL1 的桌面是挂载的 windows 的桌面 两个路径不一致. /mnt 挂在路径为真正的识别路径, 因此在下方见 /home 路径替换为真正的挂载路径
   echo -e "\e[1;33mThis WSL. Run compile_commands.json string replace : /home --> /mnt \e[0m"
   sed -i "s/\/home\/think3r\/Desktop/\/mnt\/e\/Desktop/g" ./compile_commands.json
fi
