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
    ClientServerHello *client_hello, *server_hello;
    Handshake *handshake, *server_handshake;
    RecordLayer *record, *server_message, *temp_record, record2;
    ClientKeyExchange *client_key_exchange;
    ServerKeyExchange *server_key_exchange;
    Certificate *certificate;
    CipherSuite *ciphersuite_choosen;
    Finished finished;
    Talker sender;
    int pre_master_secret_size, phase, key_block_size, enc_message_len, dec_message_len;
    unsigned int supported_ciphers;
    uint8_t **pre_master_secret;
    MD5_CTX md5;
    SHA_CTX sha;
    uint8_t *enc_message, *dec_message, *mac, *key_block, *client_write_MAC_secret, *server_write_MAC_secret, *master_secret, *sha_1, *md5_1, *sha_fin, *md5_fin, *mac_test;
    
    client_hello = NULL;
    server_hello = NULL;
    handshake = NULL;
    server_handshake = NULL;
    record = NULL;
    server_message = NULL;
    temp_record = NULL;
    client_key_exchange = NULL;
    server_key_exchange = NULL;
    certificate = NULL;
    ciphersuite_choosen = NULL;
    pre_master_secret_size = 0;
    key_block_size = 0;
    enc_message_len = 0;
    dec_message_len = 0;
    pre_master_secret = NULL;
    phase=0;
    enc_message = NULL;
    dec_message = NULL;
    mac = NULL;
    key_block = NULL;
    client_write_MAC_secret = NULL;
    server_write_MAC_secret = NULL;
    master_secret = NULL;
    sha_1 = NULL;
    md5_1 = NULL;
    sha_fin = NULL;
    md5_fin = NULL;
    mac_test = NULL;
    sender = client;
    SHA1_Init(&sha);
    MD5_Init(&md5);

    
    ///////////////////////////////////////////////////////////////PHASE 1//////////////////////////////////////////////////////////
    
    OpenCommunication(client);

    printf("Insert your favorite cipher:");
    scanf("%x", &supported_ciphers);
    if (supported_ciphers < 0x03 || (0xB <= supported_ciphers && supported_ciphers <= 0x10) || supported_ciphers>=0x17){
        perror("Not valid ciphersuite inserted.");
        exit(1);
    }
    printf("\n\n");
    uint8_t code = (uint8_t)supported_ciphers;
    
    client_hello = ClientServerHello_init(CLIENT_HELLO, 0, &code, 1);
    
    //Wrapping
    handshake = ClientServerHelloToHandshake(client_hello);
    record = HandshakeToRecordLayer(handshake);
    //Sending client hello
    sendPacketByte(record);
    printRecordLayer(record);
	
    SHA1_Update(&sha, record->message, sizeof(uint8_t)*(record->length-5));
    MD5_Update(&md5, record->message, sizeof(uint8_t)*(record->length-5));
    
    FreeRecordLayer(record);
    FreeHandshake(handshake);
    
    //Opening the communication to the server and, when authorized, reading server hello
    OpenCommunication(server);
    while(CheckCommunication() == server){}
    
    server_message = readchannel();
    printRecordLayer(server_message);
    server_handshake = RecordToHandshake(server_message);
    server_hello = HandshakeToClientServerHello(server_handshake);
    
    SHA1_Update(&sha, server_message->message, sizeof(uint8_t)*(server_message->length-5));
    MD5_Update(&md5, server_message->message, sizeof(uint8_t)*(server_message->length-5));
    
    FreeRecordLayer(server_message);
    FreeHandshake(server_handshake);
    
    
    ciphersuite_choosen = CodeToCipherSuite(server_hello->ciphersuite_code[0]);
    
    OpenCommunication(server);
    phase = 2;
    ///////////////////////////////////////////////////////////////PHASE 2//////////////////////////////////////////////////////////
    while(phase == 2){
        while(CheckCommunication() == server){}
        
        server_message = readchannel();
        printRecordLayer(server_message);
        server_handshake = RecordToHandshake(server_message);
        
        switch (server_handshake->msg_type) {
            case CERTIFICATE:
                certificate = HandshakeToCertificate(server_handshake);
                
                SHA1_Update(&sha,server_message->message, sizeof(uint8_t)*(server_message->length-5));
                MD5_Update(&md5,server_message->message, sizeof(uint8_t)*(server_message->length-5));
                
                FreeRecordLayer(server_message);
                FreeHandshake(server_handshake);
                
                OpenCommunication(server);
                break;
            case SERVER_KEY_EXCHANGE:

                server_key_exchange = HandshakeToServerKeyExchange(server_handshake, certificate);
                
                SHA1_Update(&sha, server_message->message, sizeof(uint8_t)*(server_message->length-5));
                MD5_Update(&md5, server_message->message, sizeof(uint8_t)*(server_message->length-5));
                
                Verify_(ciphersuite_choosen, client_hello, server_hello, server_key_exchange->parameters, server_key_exchange->len_parameters, server_key_exchange->signature, server_key_exchange->len_signature, certificate);
                
                FreeRecordLayer(server_message);
                FreeHandshake(server_handshake);
                
                OpenCommunication(server);
                break;
            case SERVER_DONE:
                
                SHA1_Update(&sha, server_message->message, sizeof(uint8_t)*(server_message->length-5));
                MD5_Update(&md5, server_message->message, sizeof(uint8_t)*(server_message->length-5));
                
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
        
		///CLIENT_KEY_EXCHANGE///
        pre_master_secret = (uint8_t**)calloc(1, sizeof(uint8_t*));
        client_key_exchange = ClientKeyExchange_init(ciphersuite_choosen, certificate, server_key_exchange, pre_master_secret, &pre_master_secret_size);
        handshake = ClientKeyExchangeToHandshake(client_key_exchange);
        record = HandshakeToRecordLayer(handshake);
        
        sendPacketByte(record);
        printRecordLayer(record);
        OpenCommunication(server);

        SHA1_Update(&sha,record->message, sizeof(uint8_t)*(record->length-5));
        MD5_Update(&md5,record->message, sizeof(uint8_t)*(record->length-5));
        
        FreeClientKeyExchange(client_key_exchange);
        FreeServerKeyExchange(server_key_exchange);
        FreeCertificate(certificate);
        FreeRecordLayer(record);
        FreeHandshake(handshake);

        //MASTER KEY COMPUTATION
        printf("PRE MASTER:\n");
        for (int i = 0; i<pre_master_secret_size; i++) {
            printf("%02X ", (*pre_master_secret)[i]);
        }
        printf("\n");
        
        master_secret = MasterSecretGen(*pre_master_secret, pre_master_secret_size, client_hello, server_hello);
        
        free(*pre_master_secret);
        free(pre_master_secret);
        
        //free(&pre_master_secret);//TODO:rivedere
        
        //TODO: rimuovere questi print
        printf("MASTER KEY:generated\n");
        for (int i=0; i< 48; i++){
            printf("%02X ", master_secret[i]);
        }
        printf("\n\n");
        
        //KEYBLOCK GENERATION
        key_block = KeyBlockGen(master_secret, ciphersuite_choosen, &key_block_size, client_hello, server_hello);

        printf("KEY BLOCK\n");
        for (int i=0; i< key_block_size; i++){
            printf("%02X ", key_block[i]);
        }
        printf("\n\n");
        phase = 4;
        FreeClientServerHello(client_hello);
        FreeClientServerHello(server_hello);
        
    }
    
    ///////////////////////////////////////////////////////////////PHASE 4//////////////////////////////////////////////////////////
    while(CheckCommunication() == server){};
    record = ChangeCipherSpecRecord();
    sendPacketByte(record);
    printRecordLayer(record);

    FreeRecordLayer(record);
    OpenCommunication(server);
    
    while(CheckCommunication() == server){};
    
    //building finished
    
    SHA1_Update(&sha, &sender, sizeof(uint32_t));
    MD5_Update(&md5, &sender, sizeof(uint32_t));
    
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
    MD5_Update(&md5, md5_1,sizeof(uint8_t)*16);
    
    md5_fin = calloc(16, sizeof(uint8_t));
    sha_fin = calloc(20, sizeof(uint8_t));
    
    SHA1_Final(sha_fin, &sha);
    MD5_Final(md5_fin, &md5);
    
    //Finished allocation:
    finished.hash = (uint8_t*)calloc(36, sizeof(uint8_t));
    
    memcpy(finished.hash, md5_fin, 16*sizeof(uint8_t));
    memcpy(finished.hash + 16, sha_fin, 20*sizeof(uint8_t));
    
    free(sha_1);
    free(md5_1);
    free(md5_fin);
    free(sha_fin);
    

    /* MAC and ENCRYPTION*/
    handshake = FinishedToHandshake(&finished);   
    temp_record = HandshakeToRecordLayer(handshake);
    
    free(finished.hash);
    
    //compute MAC
    client_write_MAC_secret = NULL;
    server_write_MAC_secret = NULL;
    
    client_write_MAC_secret = key_block;
    
    mac = MAC(ciphersuite_choosen, handshake, client_write_MAC_secret);
    
    uint8_t *message_with_mac;
    
    message_with_mac = (uint8_t*)calloc(temp_record->length + ciphersuite_choosen->hash_size - 5, sizeof(uint8_t));
    memcpy(message_with_mac, temp_record->message, temp_record->length);
    memcpy(message_with_mac + (temp_record->length - 5) , mac, ciphersuite_choosen->hash_size);
    free(mac);
    
    // update length
    temp_record->length = temp_record->length + ciphersuite_choosen->hash_size;
    free(temp_record->message);
	temp_record->message = message_with_mac;
    
    uint8_t length_bytes[4];
    int_To_Bytes(temp_record->length, length_bytes);
    //TODO: temp_record to record
    printf("FINISHED:to sent\n");
    printf("%02X ", temp_record->type);
    printf("%02X ", temp_record->version.major);
    printf("%02X ", temp_record->version.minor);
    printf("%02X ", length_bytes[2]);
    printf("%02X ", length_bytes[3]);
    for(int i=0; i<temp_record->length - 5; i++){
        printf("%02X ", temp_record->message[i]);
    }
    printf("\n\n");
    
    enc_message = DecEncryptPacket(temp_record->message, temp_record->length - 5, &enc_message_len, ciphersuite_choosen, key_block, client, 1);
    
    FreeRecordLayer(temp_record);
    FreeHandshake(handshake);
    
    record2.type = HANDSHAKE;
    record2.length = enc_message_len + 5;
    record2.version = std_version;
    record2.message = enc_message;
        
    sendPacketByte(&record2);

    int_To_Bytes(record2.length, length_bytes);
    printf("ENCRYPED FINISHED: sent\n");
    printf("%02X ", record2.type);
    printf("%02X ", record2.version.major);
    printf("%02X ", record2.version.minor);
    printf("%02X ", length_bytes[2]);
    printf("%02X ", length_bytes[3]);
    for(int i=0; i<record2.length - 5; i++){
        printf("%02X ", record2.message[i]);
    }
    printf("\n\n");
    
    free(record2.message);
 
    
    OpenCommunication(server);
    while(CheckCommunication() == server){};
    
    //CHANGE CIPHER SPEC
    server_message = readchannel();
    printRecordLayer(server_message);
    
    OpenCommunication(server);
    while(CheckCommunication() == server){};
    FreeRecordLayer(server_message);
    
    //FINISHED 
    server_message = readchannel();
    
    int_To_Bytes(server_message->length, length_bytes);
    printf("FINISHED ENCRYPED: received\n");
    printf("%02X ", server_message->type);
    printf("%02X ", server_message->version.major);
    printf("%02X ", server_message->version.minor);
    printf("%02X ", length_bytes[2]);
    printf("%02X ", length_bytes[3]);
    for(int i=0; i<server_message->length - 5; i++){
        printf("%02X ", server_message->message[i]);
    }
    printf("\n\n");
    
    dec_message = DecEncryptPacket(server_message->message, server_message->length - 5, &dec_message_len, ciphersuite_choosen, key_block, server, 0);
    
    int_To_Bytes(dec_message_len + 5, length_bytes);
    printf("FINISHED DECRYPTED\n");
    printf("%02X ", server_message->type);
    printf("%02X ", server_message->version.major);
    printf("%02X ", server_message->version.minor);
    printf("%02X ", length_bytes[2]);
    printf("%02X ", length_bytes[3]);
    for(int i = 0; i < dec_message_len; i++){
        printf("%02X ", dec_message[i]);
    }
    printf("\n\n");
    
    
    //MAC verification
    free(server_message->message);
    server_message->message = dec_message;
    
    handshake = RecordToHandshake(server_message);
    handshake->length = dec_message_len;

    handshake->length = handshake->length - ciphersuite_choosen->hash_size;
    handshake->msg_type = FINISHED;
    
    server_write_MAC_secret = key_block + ciphersuite_choosen->hash_size;
    
    mac = dec_message + (dec_message_len - ciphersuite_choosen->hash_size);
    
    mac_test = MAC(ciphersuite_choosen, handshake, server_write_MAC_secret);
    
    if(ByteCompare(mac, mac_test, ciphersuite_choosen->hash_size)==0){
        printf("\nmac verified\n");
    }
    else{
        printf("\nmac not verified\n");
        exit(1);
    }
    
    free(ciphersuite_choosen);
    FreeRecordLayer(server_message);
    FreeHandshake(handshake);
    free(master_secret);
    free(key_block);
    free(mac_test);
   
    return 0;
    
}
