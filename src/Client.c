#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <openssl/rand.h>
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
    X509 *cert_509;
    EVP_PKEY * pubkey;
    RSA * rsa;
    uint32_t len_parameters;
    int phase;
    uint8_t *pre_master_secret, *pre_master_secret_encrypted;
    
    //INIZIALIZZAZIONI
    server_hello = NULL;
    handshake = NULL;
    server_handshake = NULL;
    record = NULL;
    server_message = NULL;
    certificate = NULL;
    certificate_request = NULL;
    algorithm_type = 0;
    cert_509 = NULL;
    pubkey = NULL;
    rsa = NULL;
    len_parameters = 0;
    phase = 0;
    pre_master_secret = NULL;
    pre_master_secret_encrypted = NULL;
    //TODO: client_hello, random, client_key_exchange
    
    printf("!!!CLIENT AVVIATO!!!\n");
	
    ///////////////////////////////////////////////////////////////PHASE 1//////////////////////////////////////////////////////////
    OpenCommunication(client);
    
    //COSTRUZIONE CLIENT HELLO
    random.gmt_unix_time = (uint32_t)time(NULL);
    RAND_bytes(random.random_bytes, 28);
    client_hello.length = 69;
    client_hello.version = 3;
    client_hello.random = &random;
    client_hello.type = CLIENT_HELLO;

	client_hello.sessionId = 32;
    client_hello.ciphersuite = lista; //TODO: dobbiamo fare in modo da caricarle da file -> rivedere pure la lenght
				
    //WRAPPING
    handshake = ClientServerHelloToHandshake(&client_hello);
    record = HandshakeToRecordLayer(handshake);
    
    //INVIAMO e APRIAMO LA COMUNICAZIONE AL SERVER
    sendPacketByte(record);
    printf("\n!!!RECORD INVIATO: ClientHello\n");
    printf("%02X ", record->type);
    printf("%04X ", record->length);
    for(int i=0; i<record->length - 5; i++){
        printf("%02X ", record->message[i]);
        
    }
    printf("\n\n");

    OpenCommunication(server);
    
    while(CheckCommunication() == server){}
    
    server_message = readchannel();
    server_handshake = RecordToHandshake(server_message);
    server_hello = HandshakeToClientServerHello(server_handshake);//SERVER HELLO
    printf("ServerHello read!!!\n	CipherSuite:%02X\n", server_hello->ciphersuite->code);
    
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
                printf("Certificato salvato.\n");
                //TODO queste variabili andrebbero estratte dal certificato e dalla cipher suite scelta
                algorithm_type = RSA_;
                len_parameters = 128; //TODO dipende dal certificato
            	printf("Certificate read\n");
                OpenCommunication(server);
                break;
            case SERVER_KEY_EXCHANGE:
                printf("Server Key Exchange read\n");
                OpenCommunication(server);
                break;
            case CERTIFICATE_REQUEST:
                certificate_request = HandshakeToCertificateRequest(server_handshake);
                printf("CertificateRequest read\n");
                OpenCommunication(server);
                break;
            case SERVER_DONE:
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
        ///CERTIFICATE///
        
		///CLIENT_KEY_EXCHANGE///
        client_key_exchange.algorithm_type = algorithm_type;
        //TODO: da dove ricavarle??
        client_key_exchange.len_parameters = len_parameters;

		cert_509 = d2i_X509(NULL, &(certificate->X509_der), certificate->len); //converto certificato der -> X509 format
		pubkey = X509_get_pubkey(cert_509);
        printf("la chiave pubblica generica estratta\n");
        rsa = EVP_PKEY_get1_RSA(pubkey);
        printf("la chiave pubblica rsa estratta\n");
        pre_master_secret= (uint8_t*)calloc(48, sizeof(uint8_t));
        RAND_bytes(pre_master_secret, 48);
        printf("PRE-MASTER KEY:");
        for (int i=0; i< 48; i++){
            printf("%02X ", pre_master_secret[i]);
        }
        printf("\n");
        pre_master_secret_encrypted = (uint8_t*)calloc(RSA_size(rsa), sizeof(uint8_t));
    
        //cifro con RSA
        int flag = 0;
        flag = RSA_public_encrypt(48, pre_master_secret, pre_master_secret_encrypted, rsa, RSA_PKCS1_PADDING);//TODO: rivedere sto padding
        //TODO: RECALL to free EVP_PKEY_free(pubkey); and cert_509,rsa, pre_master_secret
        client_key_exchange.parameters = pre_master_secret_encrypted;
        
        handshake = ClientKeyExchangeToHandshake(&client_key_exchange);
        record = HandshakeToRecordLayer(handshake);
        
        sendPacketByte(record);
        
        printf("\nRECORD INVIATO:\n");
        printf("%02X ", record->type);
        printf("%04X ", record->length);
        for(int i=0; i<record->length - 5; i++){
            printf("%02X ", record->message[i]);
            
        }
        printf("\n\n");
        
        printf("client key exchange sent!.\n");
        OpenCommunication(server);

    	//TODO
        
        ///CERTIFICATE_VERIFY///
    	//OpenCommunication(server);
        
        //while(CheckCommunication() == server){};
        phase = 4;
    }
    /*
    ///////////////////////////////////////////////////////////////PHASE 4//////////////////////////////////////////////////////////
    while(CheckCommunication() == server){};
    
    RAND_bytes(finished.hash, 36);
    handshake = FinishedToHandshake(&finished);
    record = HandshakeToRecordLayer(handshake);
        
    sendPacketByte(record);
    printf("client finished sent.\n");
    OpenCommunication(server);
    while(CheckCommunication() == server){};
    
    server_message = readchannel();
    server_handshake = RecordToHandshake(server_message);
    server_finished = HandshakeToFinished(server_handshake);
    printf("server finished read.\n");
    
    return 0;
    */
}