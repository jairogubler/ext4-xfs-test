# Preparação

criar e montar 3 partições (fiz com uma VM adicionando 3 HDs):
mkdir -p {/xfs,/ext3,/ext4}
mkfs.ext3 /dev/sdx1
mkfs.ext4 /dev/sdx1
mkfs.xfs /dev/sdx1

mount -t ext3 /dev/sdx1 /ext3
mount -t ext4 /dev/sdx1 /ext4
mount -t xfs /dev/sdx1 /xfs

Adicionar em /etc/fstab


Depois de montar as partições:
mkdir -p {/xfs,/ext3,/ext4}/{sync,nosync}