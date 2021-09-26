echo 1
uvt-kvm create aos_vm1 release=bionic --memory=256
uvt-kvm wait aos_vm1
echo 2
uvt-kvm create aos_vm2 release=bionic --memory=256
uvt-kvm wait aos_vm2
echo 3
uvt-kvm create aos_vm3 release=bionic --memory=256
uvt-kvm wait aos_vm3
echo 4
uvt-kvm create aos_vm4 release=bionic --memory=256
uvt-kvm wait aos_vm4

