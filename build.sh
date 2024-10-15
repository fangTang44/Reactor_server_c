#/bin/bash

function cmakeRun {
	echo "开始使用cmake构建项目"
		sleep 1
	cmake ..
	if [ $? -eq 0 ]; then
		make
		if [ $? -eq 0 ]; then
			echo "编译成功"
		else 
			echo "编译失败: 项目文件存在语法错误"
			exit 1
		fi
	else 
		echo "构建失败: CMakeLists.txt存在错误请重新查看"
		exit 1
	fi
}

function build {
	if [ -e "CMakeLists.txt"  ]; then
		# 若存在build目录
		if [ -d "build" ]; then 
			cd build
		else 
			mkdir build
			if [ $? -eq 0 ]; then 
				echo "build目录创建成功"
				cd build
			else 
				echo "创建build目失败"
				exit 1
			fi
		fi
		cmakeRun
	else
		echo "CMakeLists.txt文件不存在"
		exit 1
	fi
}

function clean {
	if [ -d "build" ]; then
		cd build	
		if [ $? -eq 0 ]; then
			if [ -e "Makefile" ]; then
				make clean
			fi
			rm * -rf
			cd ..
			pwd
			rmdir build
			echo "删除成功"
		else 
			echo "未成功进入build目录"
			exit 1
		fi
	else 
		echo "为找到build目录请先编译后在执行clean操作"
		exit 1
	fi
}

function main {
	# 判断参数个数
	if [ $# -lt 1 ]; then
			echo "错误: 需要至少一个参数"
			echo "用法: $0 <build>/<clean><rebuild>"
			exit 1
	fi

	if [ $1 = "build" ]; then
		build
		exit 0
	elif [ $1 = "clean" ]; then
		clean
		exit 0
	elif [ $1 = "rebuild" ]; then
		if [ -d "build" ]; then
			clean
		fi
		build
		exit 0
	else 
		echo "错误: 不存在该参数$1"
		echo "用法: $0 <build>/<clean>/<rebuild>"
		exit 1
	fi
}

main "$@"