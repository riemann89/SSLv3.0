#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include "SSL_functions.h"


int main(int argc, const char *argv[]){
    //VARIABLE DECLARATION
    ClientServerHello client_hello, *server_hello;
    Handshake *handshake, *server_handshake;
    RecordLayer *record, *server_message;
    Random random;
    ClientKeyExchange client_key_exchange;
    Certificate *certificate;
    CertificateRequest *certificate_request;
    KeyExchangeAlgorithm algorithm_type;
    Finished finished;
    EVP_PKEY * pubkey;
    RSA * rsa;
    uint32_t len_parameters;
    int phase, key_block_size;
    uint8_t *pre_master_secret, *pre_master_secret_encrypted, *master_secret,*sha_1,*md5_1, *sha_fin, *md5_fin, *iv, *cipher_key;
    MD5_CTX md5;
    SHA_CTX sha;
    uint32_t sender_var ,*sender;
    uint8_t len_hello, *key_block;
    CipherSuite *supported_ciphers;
    CipherSuite2 *cipher_suite_choosen;
    
    
    
    //INIZIALIZZAZIONI
    server_hello = NULL;
    handshake = NULL;
    server_handshake = NULL;
    record = NULL;
    server_message = NULL;
    certificate = NULL;
    certificate_request = NULL;
    pubkey = NULL;
    rsa = NULL;
    pre_master_secret = NULL;
    pre_master_secret_encrypted = NULL;
    master_secret = NULL;
    key_block = NULL;
    iv = NULL;
    cipher_key = NULL;
    algorithm_type = 0;
    len_parameters = 0;
    len_hello = 0;
    phase = 0;
    key_block_size = 0;
    SHA1_Init(&sha);
    MD5_Init(&md5);
    
    printf("!!!CLIENT AVVIATO!!!\n");
    ///////////////////////////////////////////////////////////////PHASE 1//////////////////////////////////////////////////////////
    OpenCommunication(client);
    
    //COSTRUZIONE CLIENT HELLO
    
    random.gmt_unix_time = (uint32_t)time(NULL);
    RAND_bytes(random.random_bytes, 28);
    client_hello.version = 3;
    client_hello.random = &random;
    client_hello.type = CLIENT_HELLO;
    client_hello.sessionId = 32;
    supported_ciphers = loadCipher("ClientConfig/Priority3.txt", &len_hello); //deve restituire una lista di codici.
    client_hello.length = 38 + len_hello;
    client_hello.ciphersuite = supported_ciphers; //TODO: dobbiamo fare in modo da caricarle da file -> rivedere pure la lenght
	
    sender_var = client_hello.sessionId; //faccio sto giro se no con la free mando tutto in vacca
    sender=&sender_var;  
    
    //WRAPPING
    handshake = ClientServerHelloToHandshake(&client_hello);
    record = HandshakeToRecordLayer(handshake);
    
    //INVIAMO e APRIAMO LA COMUNICAZIONE AL SERVER
    sendPacketByte(record);
    printf("\nCLIENT HELLO: sent\n");
    for(int i=0; i<record->length - 5; i++){
        printf("%02X ", record->message[i]);
        
    }
    printf("\n\n");

    
    
    SHA1_Update(&sha,record->message,sizeof(uint8_t)*(record->length-5));
    MD5_Update(&md5,record->message,sizeof(uint8_t)*(record->length-5));
    
    FreeRecordLayer(record);
    FreeHandshake(handshake);
    
    OpenCommunication(server);
    
    while(CheckCommunication() == server){}
    
    server_message = readchannel();
    server_handshake = RecordToHandshake(server_message);
    server_hello = HandshakeToClientServerHello(server_handshake);//SERVER HELLO
    
    printf("\nSERVER HELLO: read\n");
    for(int i=0; i<server_message->length - 5; i++){
        printf("%02X ", server_message->message[i]);
        
    }
    printf("\n\n");
    SHA1_Update(&sha,server_message->message,sizeof(uint8_t)*(server_message->length-5));
    MD5_Update(&md5,server_message->message,sizeof(uint8_t)*(server_message->length-5));
    
    FreeRecordLayer(server_message);
    FreeHandshake(server_handshake);
    
    algorithm_type = getAlgorithm(server_hello->ciphersuite[0]);
    cipher_suite_choosen = CodeToCipherSuite(05);
    //TODO FreeClientServerHello(server_hello);
    ///////////////////////////////////////////////////////////////PHASE 2//////////////////////////////////////////////////////////
    OpenCommunication(server);
    phase = 2;
    while(phase == 2){
        while(CheckCommunication() == server){}
        
        server_message = readchannel();
        server_handshake = RecordToHandshake(server_message);
        
        switch (server_handshake->msg_type) {
            case CERTIFICATE:
                certificate = HandshakeToCertificate(server_handshake);
                
                printf("\nCERTIFICATE: read\n");
                for(int i=0; i<server_message->length - 5; i++){
                    printf("%02X ", server_message->message[i]);
                    
                }
                printf("\n\n");
                
                SHA1_Update(&sha,server_message->message,sizeof(uint8_t)*(server_message->length-5));
                MD5_Update(&md5,server_message->message,sizeof(uint8_t)*(server_message->length-5));
               
                 FreeRecordLayer(server_message);
                 FreeHandshake(server_handshake);
                
                len_parameters = 128; //TODO dipende dal certificato
                
                OpenCommunication(server);
                break;
            case SERVER_KEY_EXCHANGE:
                printf("TODO:SERVER KEY EXCHANGE: read\n");
                OpenCommunication(server);
                break;
            case CERTIFICATE_REQUEST:
                certificate_request = HandshakeToCertificateRequest(server_handshake);
                
                printf("\nCERTIFICATE REQUEST: read\n");
                for(int i=0; i<server_message->length - 5; i++){
                    printf("%02X ", server_message->message[i]);
                    
                }
                printf("\n\n");
                SHA1_Update(&sha,server_message->message,sizeof(uint8_t)*(server_message->length-5));
                MD5_Update(&md5,server_message->message,sizeof(uint8_t)*(server_message->length-5));
                
                FreeRecordLayer(server_message);
                FreeHandshake(server_handshake);
                //FreeCertificateRequest(certificate_request);
                
                OpenCommunication(server);
                break;
            case SERVER_DONE:
                printf("\nSERVER DONE: read\n");
                for(int i=0; i<server_message->length - 5; i++){
                    printf("%02X ", server_message->message[i]);
                    
                }
                printf("\n\n");
                
                SHA1_Update(&sha,server_message->message,sizeof(uint8_t)*(server_message->length-5));
                MD5_Update(&md5,server_message->message,sizeof(uint8_t)*(server_message->length-5));
                
                FreeRecordLayer(server_message);
                FreeHandshake(server_handshake);
            
                phase = 3;
                break;
            default:
                perror("ERROR: Unattended message in phase 2.\n");
                exit(1);
                break;
        }
    
    }
    
    ///////////////////////////////////////////////////////////////PHASE 3//////////////////////////////////////////////////////////
    while(phase == 3){
        //TODO: switch ???
        ///CERTIFICATE///
        
		///CLIENT_KEY_EXCHANGE///
        client_key_exchange.algorithm_type = cipher_suite_choosen->key_exchange_algorithm;
        //TODO: da dove ricavarle??

        pubkey = readCertificateParam(certificate);
        printf("SIZE PUBKEY: %d/n", EVP_PKEY_size(pubkey));
        client_key_exchange.len_parameters = len_parameters;
        
        pre_master_secret= (uint8_t*)calloc(48, sizeof(uint8_t));
        RAND_bytes(pre_master_secret, 48);
        pre_master_secret_encrypted= encryptPreMaster(pubkey, algorithm_type , pre_master_secret);
       
        client_key_exchange.parameters = pre_master_secret_encrypted;
        
        handshake = ClientKeyExchangeToHandshake(&client_key_exchange);
        record = HandshakeToRecordLayer(handshake);
        
        sendPacketByte(record);
        OpenCommunication(server);
        printf("\nCLIENT KEY EXCHANGE: sent.\n");
        for(int i=0; i<record->length - 5; i++){
            printf("%02X ", record->message[i]);
            
        }
        printf("\n\n");
        
        SHA1_Update(&sha,record->message,sizeof(uint8_t)*(record->length-5));
        MD5_Update(&md5,record->message,sizeof(uint8_t)*(record->length-5));
                
        FreeRecordLayer(record);
        FreeHandshake(handshake);

        //MASTER KEY COMPUTATION
        master_secret = calloc(48, sizeof(uint8_t));
        master_secret = MasterSecretGen(pre_master_secret, &client_hello, server_hello);
        
        printf("\nMASTER KEY:generated\n");
        for (int i=0; i< 48; i++){
            printf("%02X ", master_secret[i]);
        }
        printf("\n");
        
        //KEYBLOCK GENERATION
        key_block_size = 2*(cipher_suite_choosen->signature_size + cipher_suite_choosen->key_material + cipher_suite_choosen->iv_size);
        printf("key block size: %d\n", key_block_size);
        key_block = KeyBlockGen(master_secret, cipher_suite_choosen, &client_hello, server_hello);
		
        printf("\nKEY BLOCK\n");
        for (int i=0; i< key_block_size; i++){
            printf("%02X ", key_block[i]);
        }
        printf("\n");
        


        ///CERTIFICATE_VERIFY///
    	//TODO
        //OpenCommunication(server);
        
        //while(CheckCommunication() == server){};
        phase = 4;
    }
    
    ///////////////////////////////////////////////////////////////PHASE 4//////////////////////////////////////////////////////////
    while(CheckCommunication() == server){};
    record = ChangeCipherSpecRecord();
    sendPacketByte(record);
     
    printf("\nCHANGE_CHIPHER_SUITE: sent.\n");
    for(int i=0; i<record->length - 5; i++){
        printf("%02X ", record->message[i]);       
    }
    printf("\n\n");
    
    FreeRecordLayer(record);
    //TODO Free Cipher_Spec
    OpenCommunication(server);
    
    while(CheckCommunication() == server){};
    
    //building finished
    
    SHA1_Update(&sha,sender,sizeof(uint32_t));    
    MD5_Update(&md5,sender,sizeof(uint32_t));
    
    SHA1_Update(&sha,master_secret,sizeof(uint8_t)*48);
    MD5_Update(&md5,master_secret,sizeof(uint8_t)*48);  
    
    SHA1_Update(&sha, pad_1, sizeof(uint8_t)*40);
    MD5_Update(&md5, pad_1, sizeof(uint8_t)*48);
    
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
    
    memcpy(finished.hash, md5_fin, 16*sizeof(uint8_t));
    memcpy(finished.hash + 16, sha_fin, 20*sizeof(uint8_t));
    
    /* MAC and ENCRYPTION*/
    RecordLayer *temp = NULL;
    handshake = FinishedToHandshake(&finished);
    temp = HandshakeToRecordLayer(handshake);
    // MANCA IL MAC

    uint8_t *enc_message_len = NULL;
    uint8_t *enc_message = NULL;
    enc_message_len = calloc(1, sizeof(uint8_t));
    
    enc_message = DecEncryptPacket(temp->message, temp->length, enc_message_len, cipher_suite_choosen, key_block, client, 1);
    *enc_message_len = 45;
    // assembling encrypted packet
    RecordLayer record2;
    
	record2.length = *enc_message_len + 5; //TODO
    record2.type = HANDSHAKE;
    record2.version = std_version;
    record2.message = enc_message;
        
    sendPacketByte(&record2);
    
    printf("\nCLIENT FINISHED: sent.\n");
    for(int i=0; i<record2.length - 5; i++){
        printf("%02X ", record2.message[i]);
    }
    printf("\n\n");
    
    //FreeRecordLayer(record);
    //FreeHandshake(handshake);
    free(sha_1);
    free(md5_1);
    free(sha_fin);
    free(md5_fin);
     //TODO free finished
    
    OpenCommunication(server);
    while(CheckCommunication() == server){};
    
    server_message = readchannel();
    printf("\nCHANGE_CIPHER_SPEC: read\n");
    for(int i=0; i<server_message->length - 5; i++){
    	printf("%02X ", server_message->message[i]);
        }
    printf("\n\n");
    
    
    OpenCommunication(server);
    while(CheckCommunication() == server){};
    
    server_message = readchannel();

    printf("\nSERVER FINISHED : read\n");
    for(int i=0; i<server_message->length - 5; i++){
         printf("%02X ", server_message->message[i]);
    }
    printf("\n\n");
    
    
    uint8_t dec_message_len = 40;
    uint8_t *dec_message = NULL;
    
    dec_message = calloc(40, sizeof(uint8_t));
    
    dec_message = DecEncryptPacket(server_message->message, server_message->length, &dec_message_len, cipher_suite_choosen, key_block, server, 0);
    
    
    printf("\nFINISHED DECRYPTED\n");
    for(int i=0; i < dec_message_len; i++){
        printf("%02X ", dec_message[i]);
    }
    printf("\n\n");

    
    FreeRecordLayer(server_message);
    FreeHandshake(server_handshake);
    //TODO FreeFinished(server_finished);
    free(master_secret);

    return 0;
    
}
