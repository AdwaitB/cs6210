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
echo 5
uvt-kvm create aos_vm5 release=bionic --memory=256
uvt-kvm wait aos_vm5
echo 6
uvt-kvm create aos_vm6 release=bionic --memory=256
uvt-kvm wait aos_vm6
echo 7
uvt-kvm create aos_vm7 release=bionic --memory=256
uvt-kvm wait aos_vm7
echo 8
uvt-kvm create aos_vm8 release=bionic --memory=256
uvt-kvm wait aos_vm8
