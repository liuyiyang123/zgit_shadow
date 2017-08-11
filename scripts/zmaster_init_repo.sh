#!/bin/sh

###################
zProjNo=$1  # 项目ID
zProjPath=$2  # 生产机上的绝对路径
zPullAddr=$3  # 拉取代码所用的完整地址
zRemoteMasterBranchName=$4  # 源代码服务器上用于对接生产环境的分支名称
zRemoteVcsType=$5  # svn 或 git
###################

zShadowPath=/home/git/zgit_shadow
zDeployPath=/home/git/$zProjPath

if [[ "" == $zProjNo
    || "" == $zProjPath
    || "" == $zPullAddr
    || "" == $zRemoteMasterBranchName
    || "" == $zRemoteVcsType ]]
then
    exit 255
fi

if [[ 0 -lt `ls -d ${zDeployPath} | wc -l` ]]; then exit 0; fi
# 环境初始化
git clone $zPullAddr $zDeployPath
cd $zDeployPath
git config user.name "$zProjNo"
git config user.email "${zProjNo}@${zProjPath}"
printf ".svn/" > .gitignore  # 忽略<.svn>目录
git add --all .
git commit -m "__init__"
git branch -f CURRENT
git branch -f server  # 远程代码接收到server分支

# 专用于远程库VCS是svn的场景
rm -rf ${zDeployPath}_SYNC_SVN_TO_GIT
mkdir ${zDeployPath}_SYNC_SVN_TO_GIT

if [[ "svn" == $zRemoteVcsType ]]; then
    svn co $zPullAddr ${zDeployPath}_SYNC_SVN_TO_GIT  # 将 svn 代码库内嵌在 git 仓库下建一个子目录中
    cd ${zDeployPath}_SYNC_SVN_TO_GIt
    git init .
    git config user.name "sync_svn_to_git"
    git config user.email "sync_svn_to_git@${zProjNo}"
    printf ".svn/" > .gitignore
    git add --all .
    git commit -m "__init__"
fi

# 创建以 <项目名称>_SHADOW 命名的目录
rm -rf ${zDeployPath}_SHADOW 2>/dev/null
mkdir ${zDeployPath}_SHADOW

cp -rf ${zShadowPath}/bin ${zDeployPath}_SHADOW/
cp -rf ${zShadowPath}/scripts ${zDeployPath}_SHADOW/

cd ${zDeployPath}_SHADOW
git init .
git config user.name "git_shadow"
git config user.email "git_shadow@${zProjNo}"
git add --all .
git commit --allow-empty -m "__init__"

# 防止添加重复条目
zExistMark=`cat /home/git/zgit_shadow/conf/master.conf | grep -Pc "^\s*${zProjNo}\s*"`
if [[ 0 -eq $zExistMark ]];then
    echo "${zProjNo} ${zProjPath} ${zPullAddr} ${zRemoteMasterBranchName} ${zRemoteVcsType}" >> ${zShadowPath}/conf/master.conf
fi

# 创建必要的目录与文件
cd $zDeployPath
mkdir -p ${zDeployPath}_SHADOW/{info,log/deploy}
touch ${zDeployPath}_SHADOW/info/{host_ip_all.bin,host_ip_all.txt,host_ip_major.txt,repo_id}
touch ${zDeployPath}_SHADOW/log/deploy/meta
chmod -R 0755 ${zDeployPath}_SHADOW