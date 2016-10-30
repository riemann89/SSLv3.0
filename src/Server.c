//
//  Server.c
//  SSLv3.0
//
//  Created by Giuseppe Giffone on 16/02/16.
//  Copyright © 2016 Giuseppe Giffone. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <openssl/dh.h>
#include <openssl/bn.h>
#include "SSL_functions.h"
#include "Utilities.h"

int main(int argc, const char *argv[]){
    //Declaration
    ClientServerHello server_hello, *client_hello;
    Handshake *handshake, *client_handshake;
    RecordLayer *record, *client_message, *temp;
    ClientKeyExchange *client_key_exchange;
    Random random;
    Certificate *certificate;
    CertificateVerify *certificate_verify;
    Finished finished;
    CipherSuite *ciphersuite_choosen;
    CertificateType certificate_type;
    Talker sender;
    int phase, key_block_size, len_parameters,dec_message_len,enc_message_len;

    uint8_t prioritylen, ciphersuite_code, *pre_master_secret, *master_secret,*sha_1, *md5_1, *sha_fin, *md5_fin, session_Id[4];
    MD5_CTX md5;
    SHA_CTX sha;
    uint8_t *cipher_key, server_write_MAC_secret[16], client_write_MAC_secret[16];
    uint8_t *key_block,*dec_message,*enc_message,*mac,*mac2;
	DH *dh;
    BIGNUM *pub_key_client;
    size_t out_size;
    ServerKeyExchange server_key_exchange;
    size_t pre_master_secret_size;
	
    EVP_PKEY *private_key;
    
    //Initialization
    master_secret = NULL;
    cipher_key = NULL;
    key_block = NULL;
    certificate = NULL;
    dec_message = NULL;
    enc_message=NULL;
    dh = NULL;
    ciphersuite_code = 0;
    prioritylen = 10;
    phase = 0;
    certificate_type = 0;
    dec_message_len = 0;
    enc_message_len = 0;
    out_size = 0;
    pre_master_secret_size = 0;
    sender = server;
    SHA1_Init(&sha);
    MD5_Init(&md5);
    
    ///////////////////////////////////////////////////////////////PHASE 1//////////////////////////////////////////////////////////
    while(CheckCommunication() == client){}
    client_message = readchannel();
    
    
    printRecordLayer(client_message);
    
    SHA1_Update(&sha, client_message->message, sizeof(uint8_t)*(client_message->length-5));
    MD5_Update(&md5, client_message->message, sizeof(uint8_t)*(client_message->length-5));

    client_handshake = RecordToHandshake(client_message);
    client_hello = HandshakeToClientServerHello(client_handshake);
   
	
    //TODO: ciphersuite_code = chooseChipher(client_hello, "ServerConfig/All.txt");
    ciphersuite_code = 1;
    
    //Construction Server Hello
    random.gmt_unix_time = (uint32_t)time(NULL);
    RAND_bytes(random.random_bytes, 28);
	
    server_hello.type = SERVER_HELLO;
    server_hello.length = 39;//perchè è 39?
    server_hello.version = 3;
    server_hello.random = &random;
    server_hello.sessionId = Bytes_To_Int(4, session_Id);
    server_hello.ciphersuite_code = &ciphersuite_code;
				
    //Wrapping
    handshake = ClientServerHelloToHandshake(&server_hello);
    record = HandshakeToRecordLayer(handshake);
    
    SHA1_Update(&sha,record->message, sizeof(uint8_t)*(record->length-5));
    MD5_Update(&md5,record->message, sizeof(uint8_t)*(record->length-5));
    
    //ciphersuite_choosen = CodeToCipherSuite(ciphersuite_code); TODO: eliminare la riga dopo usata per i test
    
    ciphersuite_choosen = CodeToCipherSuite(0x14); //TODO: riga su...
    certificate_type = CodeToCertificateType(0x14);//TODO: automatizzare
    
    //Sending server hello and open the communication to the client.
    sendPacketByte(record);
    printRecordLayer(record);
    OpenCommunication(client);
    
    ///////////////////////////////////////////////////////////////PHASE 2//////////////////////////////////////////////////////////
    while(CheckCommunication() == client){}
    
    //CERTIFICATE
    switch (ciphersuite_choosen->key_exchange_algorithm){
        case RSA_:
            //strcpy((char*)&certificate_string, "certificates/RSA_server.crt");
            certificate = loadCertificate("certificates/RSA_server.crt");
            break;
        case DH_:
            switch (ciphersuite_choosen->signature_algorithm) {
                case RSA_s:
                    certificate = loadCertificate("certificates/RSA_server.crt");
                    break;
                case DSA_s:
                    certificate = loadCertificate("certificates/DSA_server.crt");
                    break;
                default:
                    break;
            }
            break;
        default:
            perror("Certificate error: type not supported.");
            exit(1);
            break;
        }

    handshake = CertificateToHandshake(certificate);
    record = HandshakeToRecordLayer(handshake);
       
    SHA1_Update(&sha,record->message,sizeof(uint8_t)*(record->length-5));
    MD5_Update(&md5,record->message,sizeof(uint8_t)*(record->length-5));
    
    sendPacketByte(record);
    printRecordLayer(record);
    OpenCommunication(client);
    while(CheckCommunication() == client){}
	
    
    //SERVER KEY EXCHANGE
    if (ciphersuite_choosen->key_exchange_algorithm == DH_){
        
        //dh = DH_new();//TODO: remember to free
        dh = get_dh2048();
        if(DH_generate_key(dh) == 0){
            perror("DH keys generarion error.");
            exit(1);
        }
        
        server_key_exchange.len_parameters = BN_num_bytes(dh->p) + BN_num_bytes(dh->g) + BN_num_bytes(dh->pub_key);
        //TODO: questi mi sa che non vanno allocati
        server_key_exchange.parameters = (uint8_t*)calloc(server_key_exchange.len_parameters, sizeof(uint8_t));
        
        BN_bn2bin(dh->p, server_key_exchange.parameters);
        BN_bn2bin(dh->g, server_key_exchange.parameters + BN_num_bytes(dh->p));
        BN_bn2bin(dh->pub_key, server_key_exchange.parameters + BN_num_bytes(dh->p) + BN_num_bytes(dh->g));

    	//TODO rivedere l'inizializzazione delle variabili
        private_key = EVP_PKEY_new();
        FILE *key_file;
        key_file = NULL;
        
        switch (ciphersuite_choosen->signature_algorithm) {
            case RSA_s:
                key_file = fopen("private_keys/RSA_server.key","rb");
                break;
            case DSA_s:
                key_file = fopen("private_keys/DSA_server.key","rb");
            default:
                perror("Error private key.");
                exit(1);
                break;
        }
		
        private_key = PEM_read_PrivateKey(key_file, &private_key, NULL, NULL);
        server_key_exchange.signature = Signature_(ciphersuite_choosen, client_hello, &server_hello, server_key_exchange.parameters, server_key_exchange.len_parameters, private_key);
        
        printf("%d\n", EVP_PKEY_size(private_key));
        server_key_exchange.len_signature = EVP_PKEY_size(private_key);
        
        handshake = ServerKeyExchangeToHandshake(&server_key_exchange);
        record = HandshakeToRecordLayer(handshake);
        
        SHA1_Update(&sha,record->message,sizeof(uint8_t)*(record->length-5));
        MD5_Update(&md5,record->message,sizeof(uint8_t)*(record->length-5));

        sendPacketByte(record);
        printRecordLayer(record);
        OpenCommunication(client);
        while(CheckCommunication() == client){}
    }
    
    //CERTIFICATE REQUEST
    
    //SERVER HELLO DONE
    handshake = ServerDoneToHandshake();
    record = HandshakeToRecordLayer(handshake);
    
    SHA1_Update(&sha,record->message,sizeof(uint8_t)*(record->length-5));
    MD5_Update(&md5,record->message,sizeof(uint8_t)*(record->length-5));
    
    sendPacketByte(record);
    printRecordLayer(record);
    OpenCommunication(client);
    
    ///////////////////////////////////////////////////////////////PHASE 3//////////////////////////////////////////////////////////
    
    phase = 3;
    while(phase == 3){
        while(CheckCommunication() == client){}
        
        client_message = readchannel();
        printRecordLayer(client_message);
        client_handshake = RecordToHandshake(client_message);
        switch (client_handshake->msg_type) {
                case CERTIFICATE:
                    certificate = HandshakeToCertificate(client_handshake);
                    OpenCommunication(client);
                    break;
                case CLIENT_KEY_EXCHANGE:
                    len_parameters = client_handshake->length - 4;
                    client_key_exchange = HandshakeToClientKeyExchange(client_handshake);

                	switch (ciphersuite_choosen->key_exchange_algorithm){
                    	case RSA_:
                            private_key = EVP_PKEY_new();
                            FILE *key_file;
                            key_file = NULL;
                            key_file = fopen("private_keys/RSA_server.key","rb");
                            private_key = PEM_read_PrivateKey(key_file, &private_key, NULL, NULL);
                            pre_master_secret = AsymDec(EVP_PKEY_RSA, client_key_exchange->parameters, len_parameters, &pre_master_secret_size, private_key);
                        	break;
                        case DH_:
                            pub_key_client = BN_new();
                            pub_key_client = BN_bin2bn(client_key_exchange->parameters, DH_size(dh), NULL);
                            
                            pre_master_secret = (uint8_t*)calloc(DH_size(dh), sizeof(uint8_t));
                            pre_master_secret_size = DH_compute_key(pre_master_secret, pub_key_client, dh);
                            break;
                    	default:
                            perror("Client Key Exchange not supported");
                            exit(1);
                        	break;
                	}
                printf("PRE MASTER:\n");
                for (int i = 0; i<pre_master_secret_size; i++) {
                    printf("%02X ",pre_master_secret[i]);
                }
                printf("\n");
                
                
                master_secret = MasterSecretGen(pre_master_secret, 48, client_hello, &server_hello);
                
                SHA1_Update(&sha,client_message->message, sizeof(uint8_t)*(client_message->length-5));
                MD5_Update(&md5,client_message->message, sizeof(uint8_t)*(client_message->length-5));

                printf("MASTER KEY:generated\n");
                for (int i=0; i< 48; i++){
                    printf("%02X ", master_secret[i]);
                }
                printf("\n");
                    
                //KEYBLOCK GENERATION
                key_block = KeyBlockGen(master_secret, ciphersuite_choosen, &key_block_size, client_hello, &server_hello);
                    
                printf("\nKEY BLOCK\n");
                for (int i=0; i< key_block_size; i++){
                    printf("%02X ", key_block[i]);
                }
                printf("\n\n");
                phase = 4;//TODO: se usa il verify non funziona
                OpenCommunication(client);

                    break;
                case CERTIFICATE_VERIFY:
                    certificate_verify = HandshakeToCertificateVerify(client_handshake);
                    
                    SHA1_Update(&sha,client_message->message,sizeof(uint8_t)*(client_message->length-5));
                    MD5_Update(&md5,client_message->message,sizeof(uint8_t)*(client_message->length-5));
                    OpenCommunication(client);
                    break;
            	default:
                    perror("ERROR: Unattended message in phase 3.\n");
                    exit(1);
                    break;
            }
        
    }
    ///////////////////////////////////////////////////////////////PHASE 4//////////////////////////////////////////////////////////
    
    //CHANGE CIPHER SPEC read
    while(CheckCommunication() == client)
    client_message = readchannel();
    
    printRecordLayer(client_message);
    
    //FINISHED read
    OpenCommunication(client);
    while(CheckCommunication() == client){}
    
    client_message = readchannel();
    
    uint8_t length_bytes[4];
    int_To_Bytes(client_message->length, length_bytes);
    printf("ENCRYPED FINISHED: received\n");
    printf("%02X ", client_message->type);
    printf("%02X ", client_message->version.major);
    printf("%02X ", client_message->version.minor);
    printf("%02X ", length_bytes[2]);
    printf("%02X ", length_bytes[3]);
    for(int i=0; i<client_message->length - 5; i++){
        printf("%02X ", client_message->message[i]);
    }
    printf("\n\n");
    
    dec_message = DecEncryptPacket(client_message->message, client_message->length - 5, &dec_message_len, ciphersuite_choosen, key_block, client, 0);
    
    int_To_Bytes(dec_message_len + 5, length_bytes);
    printf("%d\n", dec_message_len);
    printf("DECRYPTED FINISHED:\n");
    printf("%02X ", client_message->type);
    printf("%02X ", client_message->version.major);
    printf("%02X ", client_message->version.minor);
    printf("%02X ", length_bytes[2]);
    printf("%02X ", length_bytes[3]);
    for(int i = 0; i < dec_message_len; i++){
        printf("%02X ", dec_message[i]);
    }
    printf("\n\n");

    //TODO MAC verification
    client_message->message=dec_message;
    handshake= RecordToHandshake(client_message);
            
    if(ciphersuite_choosen->signature_algorithm == SHA1_){
        handshake->length= handshake->length - 20;
    }
    else if(ciphersuite_choosen->signature_algorithm==MD5_1){
        handshake->length= handshake->length - 16;
    }       
    mac2 = &handshake->content[handshake->length - 4];
    
    
    for(int i=0;i<16; i++){
        client_write_MAC_secret[i]=key_block[i];
    }
    mac= MAC(ciphersuite_choosen,handshake,client_write_MAC_secret);

    // TODO now i should compare mac 1 and mac2 they should be equal
    
    //CHANGE CIPHER SPEC send
    record = ChangeCipherSpecRecord();
    sendPacketByte(record);
    printRecordLayer(record);
    
    OpenCommunication(client);
    
    while(CheckCommunication() == client){}
    
    //FINISHED send
    SHA1_Update(&sha, &sender, sizeof(uint32_t));
    MD5_Update(&md5, &sender, sizeof(uint32_t));
    
    SHA1_Update(&sha,master_secret,sizeof(uint8_t)*48);
    MD5_Update(&md5,master_secret,sizeof(uint8_t)*48);  
    
    SHA1_Update(&sha, pad_1,sizeof(uint8_t)*40);
    MD5_Update(&md5, pad_1,sizeof(uint8_t)*48);
    
    md5_1 = calloc(16, sizeof(uint8_t));
    sha_1 = calloc(20, sizeof(uint8_t));
    
    SHA1_Final(sha_1,&sha);
    MD5_Final(md5_1,&md5);
    
    SHA1_Init(&sha);
    MD5_Init(&md5);
    
    SHA1_Update(&sha, master_secret,sizeof(uint8_t)*48);
    SHA1_Update(&sha, pad_2,sizeof(uint8_t)*40);
    SHA1_Update(&sha, sha_1,sizeof(uint8_t)*20);
    
    MD5_Update(&md5, master_secret,sizeof(uint8_t)*48);
    MD5_Update(&md5, pad_2,sizeof(uint8_t)*48);
    MD5_Update(&md5, sha_1,sizeof(uint8_t)*16);
    
    md5_fin = calloc(16, sizeof(uint8_t));
    sha_fin = calloc(20, sizeof(uint8_t));
    
    SHA1_Final(sha_fin, &sha);
    MD5_Final(md5_fin, &md5);
    
    finished.hash = (uint8_t*)calloc(36, sizeof(uint8_t));
    
    memcpy(finished.hash, md5_fin, 16*sizeof(uint8_t));
    memcpy(finished.hash + 16, sha_fin, 20*sizeof(uint8_t));
    
    /* MAC and ENCRYPTION*/
    handshake = FinishedToHandshake(&finished);
    temp = HandshakeToRecordLayer(handshake);
    
    int_To_Bytes(temp->length, length_bytes);
    printf("FINISHED:to sent\n");
    printf("%02X ", temp->type);
    printf("%02X ", temp->version.major);
    printf("%02X ", temp->version.minor);
    printf("%02X ", length_bytes[2]);
    printf("%02X ", length_bytes[3]);
    for(int i=0; i<temp->length - 5; i++){
        printf("%02X ", temp->message[i]);
    }
    printf("\n\n");
    
    //compute MAC
    
    for(int i=0;i<16; i++){
        server_write_MAC_secret[i]=key_block[i+16];
    }
    mac= MAC(ciphersuite_choosen,handshake,server_write_MAC_secret);

    //append MAC
    for(int i=0;i<sizeof(mac);i++){
        temp->message[temp->length - 5 + i]=mac[i];
    }
    
    // update length
    temp->length= temp->length + sizeof(mac);
       
    enc_message = DecEncryptPacket(temp->message, temp->length - 5, &enc_message_len, ciphersuite_choosen, key_block, server, 1);
 
    record = calloc(1, sizeof(RecordLayer));
    record->type = HANDSHAKE;
    record->version = std_version;
    record->length = enc_message_len + 5; //TODO
    record->message = enc_message;
    
    sendPacketByte(record);
    
    int_To_Bytes(record->length, length_bytes);
    printf("ENCRYPED FINISHED: sent\n");
    printf("%02X ", record->type);
    printf("%02X ", record->version.major);
    printf("%02X ", record->version.minor);
    printf("%02X ", length_bytes[2]);
    printf("%02X ", length_bytes[3]);
    for(int i=0; i<record->length - 5; i++){
        printf("%02X ", record->message[i]);
    }
    printf("\n\n");
    
    OpenCommunication(client);

   	
	return 0;
}
