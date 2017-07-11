#!/usr/bin/env bash

zInitEnv() {
    zProjName=$1
    zSvnServPath=/home/git/svn_$zProjName  #Subversion repo to receive code from remote developers
    zSyncPath=/home/git/sync_$zProjName  #Sync snv repo to git repo
    zDeployPath=/home/git/$zProjName #Used to deploy code! --CORE--
    zSshKeyPath=$zSyncPath/.git_shadow/authorized_keys  #store Control Host and major ECSs' SSH pubkeys

    cp -r ../demo/$zProjName /home/git/
    cp -r ../demo/${zProjName}_shadow /home/git/

    #Init Subversion Server
    rm -rf $zSvnServPath
    mkdir -p $zSvnServPath
    svnadmin create $zSvnServPath
    perl -p -i.bak -e 's/^#\s*anon-access\s*=.*$/anon-access = write/' $zSvnServPath/conf/svnserve.conf
    svnserve --listen-port=$2 -d -r $zSvnServPath

   #Init svn repo
    svn co svn://127.0.0.1:$2/ $zSyncPath
    svn propset svn:ignore '.git
    .gitignore' $zSyncPath

    #Init Sync Git Env
    mkdir -p $zSyncPath/.git_shadow
    touch $zSshKeyPath
    chmod 0600 $zSshKeyPath

    cd $zSyncPath
    git init .
    echo ".svn" > .gitignore
    git config --global user.email git_shadow@yixia.com
    git config --global user.name git_shadow

    touch README
    git add --all .
    git commit -m "INIT"
    git branch -M master sync_git  # 此git的作用是将svn库代码转换为git库代码

    #Init Deploy Git Env
    mkdir -p $zDeployPath
    cd $zDeployPath
    git init .
    touch README
    git add --all .
    git commit -m "INIT"
    git tag CURRENT
    git branch server  #Act as Git server

    printf "#!/bin/sh 
export PATH=\"/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin\" &&
export HOME=\"/home/git\" &&

cd $zSyncPath &&
svn update &&
git add --all . &&
git commit -m \"Repository: \$1\\nReversion: \$2\" &&
git push --force ${zDeployPath}/.git sync_git:server" > $zSvnServPath/hooks/post-commit

    chmod 0777 $zSvnServPath/hooks/post-commit

    printf "#!/bin/sh 
export PATH=\"/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin\" &&
export HOME=\"/home/git\" &&

cd $zDeployPath &&
git pull --force ${zDeployPath}/.git server:master &&
rm -f .git_shadow &&
ln -sv /home/git/${zProjName}_shadow ${zDeployPath}/.git_shadow" > $zDeployPath/.git/hooks/post-receive

    chmod 0777 $zDeployPath/.git/hooks/post-receive
}

killall svnserve
rm -rf /home/git/*
zInitEnv miaopai 50000
#yes|ssh-keygen -t rsa -P '' -f /home/git/.ssh/id_rsa

rm -rf /tmp/miaopai
mkdir -p /tmp/miaopai
cd /tmp/miaopai
svn co svn://127.0.0.1:50000
cp /etc/* ./ 2>/dev/null
svn add *
svn commit -m "etc files"
svn up

#zInitEnv yizhibo 60000
