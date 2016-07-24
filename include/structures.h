#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>

/*****************************************STRUCTS***********************************************/
#ifndef structure_h
#define structure_h

//OTHER STRUCTS
typedef enum{client, server
}Talker;

// HANDSHAKE STRUCTS //

typedef enum{
    HELLO_REQUEST,CLIENT_HELLO, SERVER_HELLO, CERTIFICATE=11,SERVER_KEY_EXCHANGE, CERTIFICATE_REQUEST, SERVER_DONE,CERTIFICATE_VERIFY, CLIENT_KEY_EXCHANGE,FINISHED=20
}HandshakeType;

typedef struct{
    uint8_t code;
    char name[50];
}CipherSuite;

typedef struct {
    HandshakeType msg_type;
    uint32_t length;   //on file it will be  only 3 bytes (uint24)  so remember to convert
    uint8_t* content;
}Handshake;

typedef struct{
    uint32_t gmt_unix_time;
    uint8_t random_bytes[28];
}Random;

//content
typedef struct{
    uint8_t length;  //uint8_t beacause the maximum length will < 256;    38 + #ciphersuite*4 ,   WARNING: in the handshake there will be no more this byite 
    uint8_t version;
    Random random; //TODO sostituire con un puntatore (bisogna risistemare tutte le che lavorano con client_hello funzioni)
    uint32_t sessionId;
    CipherSuite *ciphersuite;
    HandshakeType type;
}ClientServerHello;

typedef struct{
}HelloRequest;

typedef struct{
    uint8_t *X509_der;
    int len;
}Certificate;

typedef enum{
    RSA_SIGN, DSS_SIGN, RSA_FIXED_DH,
    DSS_FIXED_DH,RSA_EPHEMERAL_DH, DSS_EPHEMERAL_DH,FORTEZZA_MISSI=20
}CertificateType;

typedef enum{RSA_, DIFFIE_HELLMAN, FORTEZZA}KeyExchangeAlgorithm;

typedef enum{SHA1_, MD5}SignatureAlgorithm;

typedef struct{
    CertificateType certificate_type;
    char *certificate_authorities;
    int name_lenght;    //each certificate authority name is represented using the same number of char.
    int list_length;    //number of certificate of acceptable certificate authorities.
}CertificateRequest;//TODO la struttura deve essere rivista

typedef struct{
}ServerDone;

typedef struct{
    SignatureAlgorithm algorithm_type;
    uint8_t *signature;
}CertificateVerify;

typedef struct{
    SignatureAlgorithm algorithm_type;
    uint8_t *signature;
}Finished;


// RECORD LAYER STRUCTS//

typedef struct {
    uint8_t major, minor;
}ProtocolVersion;

typedef enum{CHANGE_CIPHER_SPEC=20, ALERT, HANDSHAKE, APPLICATION_DATA
} ContentType;

typedef struct {
    ContentType type;
    ProtocolVersion version;
    uint16_t length;
    uint8_t* message;
}RecordLayer;

//Structs for ServerKeyExchange

typedef struct {
    KeyExchangeAlgorithm algorithm_type;
    int size; //TODO rivedere cosa indica precisamente
    uint8_t *parameters;
}KeyExchangeParameters;

typedef struct {//TODO
    SignatureAlgorithm algorithm_type;
    int size;
    uint8_t *signature;
}KeyExchangeSignatures;

typedef struct{//TODO
    KeyExchangeAlgorithm algorighm_type;
    uint8_t *key_exchange;
    int len_key_exchange;
}ClientKeyExchange;

typedef struct{
    KeyExchangeParameters *parameters;
    KeyExchangeSignatures *signature;
}ServerKeyExchange;

//Extern variables
extern CipherSuite lista[31];
extern ProtocolVersion std_version;

CipherSuite get_cipher_suite(uint8_t id);
#endif







