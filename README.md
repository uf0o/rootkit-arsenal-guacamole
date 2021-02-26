# rootkit-arsenal-guacamole
An attempt to restore and adapt to modern Win10 version the Rootkit Arsenal original code samples
All projects have been ported to x64 and tested on latest Win10 (2004 - 19041.746)

## Projects 

### Templates

* **KMD** : Kernel Mode Driver template that includes a userland C&C template 
* **IRQL** : multicore synchronization primimitives via rising IRQL through DPCs  
* **ReadPE** : Parse PE IAT 

### Userland Hooking
* **RemoteThread** : CreateRemoteThread for DLL injection | ported to x64 + DLL to be injected as argument
* **IATHooking** : DLL that perform IAT hooking on a given function

### Kernel Hooking
[underway]
