# Repacking The Games

## Challenges

The game files are stored in ISO disc images, primarily within .BIN archives (also known as CFC.DIG).
These files are sensitive to changes and even minor careless modifications may cause crashes.
In NSUNI, these archives are wrapped in .PGD containers and encrypted. The integrity of their entries is verified using MD5 hashes.
The internal structure of these archives is fixed. Additionally, audio archives or even individual audio files (in NSLAR) must be located at specific
LBA positions on the disc. Assets are also expected not to exceed certain size limits. Some of these constraints can be bypassed by patching the game's executable. See [here](https://bit.ly/rcjn-cli).


## General Steps

 1. Open the ISO with UMDGen.
 2. Export the File List:
    Go to File -> File List -> Export to save a text list
    of all files in the ISO.
 3. Extract and Modify:
    You can now extract, edit, and replace any
    files you want. Note:
    - In NSUNI, .BIN files (except audio archives) are encrypted and need to be decrypted first.
    - After editing the ISO, you may need to adjust the file list.
        The list has two columns: the LBA position of a file and its name.
    - The LBA positions of some primary files like EBOOT.BIN must be strictly preserved.
    - The positions of the individual AT3 files in NSLAR (or audio archives in NSUNI) must be preserved or the EBOOT.BIN must be patched.
    - Assets should be kept within the certain RAM limits:
        - Loaded Sound Banks (PHD/PBD): < 4 MB total
        - AT3 Dialog Files: < 130 KB each
        - All Other Loaded Files: < 17 MB total
    - If you change DIG/BIN archives, you must update the .md5 files or patch the EBOOT.BIN.
    - In NSUNI, you would also need to encrypt the archives back or patch the executable. 
 4. Re-import the File List:
     After making changes, go to File List ->
     Import, select the list and click Yes.
 5. Save the new disc image:
    File -> Save As -> uncompressed  .iso
