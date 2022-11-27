# 概要
いつもBootLoaderのビルド方法を忘れるのでメモ

# BootLoaderのビルド方法
`edksetup.sh`はシンボリックリンクを貼っているので、edk2でやればよい

``` bash
pushd ~/edk2
source ./edksetup.sh
build
popd
```

# Kernelのビルド方法
ターミナル起動１階に付き、`sourde buildenv.sh`を実行する必要がある。
``` bash
pushd ~/osbook/devenv
source buildenv.sh
popd
./make_image.sh
```