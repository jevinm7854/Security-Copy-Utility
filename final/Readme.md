- Name: Jevin Modi

- List of files- purenc.c, purdec.c, Makefile

- Overview of the work-

I have created a file encryption/decryption/ transmission suite similar to scp utility on UNIX. I have used gcrypt libraries for the encryption and decryption. purenc.c is built for encryption and sending the data over the network whereas purdec.c is built to listen to incoming connections and decypt the data. The programs can be run in two modes- local mode ( - l ) or over hte network( - d ). Let me talk more about the modes-
1) Local mode- When running the program over this mode, purenc will encrypt the given file and store the encrypted file locally (with .pur extension) on the same directory as the code. purdec will take a .pur file as an input and decrypt it and store it in the same directory as the purdec code. 
2) Network mode- When running on this mode, purenc will encrypt the file and send it over the network on the ip:port specified as the command line argument. The encrypted file will not be stored locally. purdec will be listening on a port specified as the command line argument. It will receive the file and decrypt it and store it in the same directory as purdec. 
To store the encrypted file locally as well as send it over the network, both the -d and -l flags need to be used. One thing to note is if a file of the same name already exists, the programs would not over write those files and return with an error saying that the file already exists. 

To run the code- 
purenc <input file> [-d <output IP-addr:port>] [-l]
purdec [-l <input file>] <port>

Purenc- The first input to purenc should be the filename. second input can be either -d or -l. if using the -d then you need to specify
ip:port. You can also have both -d and -l
Purdec- It will either take in -l and the input file which will be with .pur extension or a port number. 

- Description of code and code layout

purenc.c -
I first take in the command line inputs and store it for further use like the filename, ip and port or the mode in which the user is running  
this program. Then I read the file and store its contents into the buffer. The user is then asked to input a password to encrypt the file. I
then generate a random salt. Then a key is generated using the salt and the password. It is generated using PBKDF2. I have used SHA256 as 
the hash algorithm. Now I have the key, the next goal would be to encrypt the data. For encryption I use AES256-CBC. I then set the key for
AES and generate a random iv. But then I realized that the plaintext should be the size of a full block. Basically, the length of plaintext
should be a multiple of the block size. So we need to pad the remaining bytes. So I pad the remaining bytes with the number of padded bytes.
If I was padding last 4 bytes, I would have \x04 \x04 \x04 \x04 (4 counts of 04). And now we are ready to encrypt, using the iv and padded
plain text. The key was already set for the cipher before. Next thing is integrity. So we need a HMAC for this. I have used SHA256 for the
HMAC as well. My design choice has been encrypt-then-HMAC. This is particularly better way since, if the password the user entered is wrong,
we do not need decrypt. We can first check the HMAC and only if it passes we go ahead and decrypt. I have used the same key generated earlier
for the HMAC and encryption. I add the iv before the ciphertext and then HMAC the entire chunk. Now I have the salt, iv, ciphertext and HMAC
that the decryption program will need. So, I need a way to store it in the file or send it on the network. This is another design choice that
I made. I added salt + hmac + iv + ciphertext (in this order) and stored it in the buffer (concatenated_data). So if anyone has this buffer
and knows the correct password will be able to decrypt the file. At the same time, even if someone intercepts this buffer, they would not be
able to do anything with it without knowing the correct password. I then write concatenated_data buffer to filename.pur if in local mode or
send it over the network to purdec. When the program is running on the network mode, after the conection is established, I send the length of
the filename, then the filename, then the length of concatenated_data and then the concatenated_data.

purdec.c



