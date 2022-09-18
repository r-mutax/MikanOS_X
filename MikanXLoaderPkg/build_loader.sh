# スクリプトのディレクトリを保持
self_dir=$(cd $(dirname $0); pwd)
rm ./bin/BOOTX64.EFI

# ブートローダのビルド
pushd $HOME/edk2
./edksetup.sh
build
popd

cp $HOME/edk2/Build/MikanXLoaderX64/DEBUG_CLANG38/X64/Loader.efi ./bin/BOOTX64.EFI

# ブートローダの起動テスト
cd ./bin

rm disk.img

qemu-img create -f raw disk.img 200M
mkfs.fat -n 'MUTAX_OS' -s 2 -f 2 -R 32 -F 32 disk.img
mkdir -p mnt
sudo mount -o loop disk.img mnt
sudo mkdir -p mnt/EFI/BOOT
sudo cp BOOTX64.EFI mnt/EFI/BOOT/BOOTX64.EFI
sudo umount mnt

qemu-system-x86_64 -drive if=pflash,file=$HOME/osbook/devenv/OVMF_CODE.fd -drive if=pflash,file=$HOME/osbook/devenv/OVMF_VARS.fd -hda disk.img