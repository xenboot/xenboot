# Xen VM creation / destruction optimizations

1. Daemon enabled parallel boot: integrated in libxl, to use it:

```shell
xl server &
xl create <config file 1> <config file 2> <...>
```

2. Xenstore cache: integrated in libxl, to use it edit `tools/Rules.mk`:
```
ENABLE_XS_SUB=y
```

3. Local NUM scrubbing: integrated in the hypervisor, to use it go to the hypervisor source folder (`xen`) and type `make menuconfig` then:

`Common features` -> `Memory on domain destruction scrubbed by local CPU`
