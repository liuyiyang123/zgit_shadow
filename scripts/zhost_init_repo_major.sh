#!/bin/sh
zMajorAddr=$1
zPathOnHost=$(echo $2 | sed -n 's%/\+%/%p')

zSelfPid=$$  # 获取自身PID
zTmpFile=`mktemp /tmp/${zSelfPid}.XXXXXXXX`
echo $zSelfPid > $zTmpFile

(sleep 5; if [[ "" != `cat $zTmpFile` ]]; then kill -9 $zSelfPid; fi; rm $zTmpFile) &  # 防止遇到无效IP时长时间卡住

ssh $zMajorAddr "
    mkdir -p ${zPathOnHost}_SHADOW &&
    mkdir -p ${zPathOnHost} &&
\
    cd ${zPathOnHost}_SHADOW &&
    git init . &&
    git config user.name "git_shadow" &&
    git config user.email "git_shadow@$x" &&
    git commit --allow-empty -m "__init__" &&
    git branch -f server &&
\
    cd $zPathOnHost &&
    git init . &&
    git config user.name "MajorHost" &&
    git config user.email "MajorHost@$x" &&
    git commit --allow-empty -m "__init__" &&
    git branch -f server
    "

echo "" > $zTmpFile  # 提示后台监视线程已成功执行，不要再kill，防止误杀其它进程
