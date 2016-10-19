#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "structures.h"
#include "Utilities.h"
#include "openssl/x509.h"
#include "openssl/pem.h"

/* CONNECTION */

void OpenCommunication(Talker talker);
Talker CheckCommunication();
void sendPacketByte(RecordLayer *record_layer);
RecordLayer *readchannel();

/* PACKET MANAGING */

//message -> handshake
Handshake *HelloRequestToHandshake();
Handshake *ClientServerHelloToHandshake(ClientServerHello *client_server_hello);
Handshake *CertificateToHandshake(Certificate *certificate);
Handshake *CertificateRequestToHandshake(CertificateRequest *certificate_request);
Handshake *ServerDoneToHandshake();
Handshake *CertificateVerifyToHandshake(CertificateVerify *certificate_verify);
Handshake *ClientKeyExchangeToHandshake(ClientKeyExchange *client_server_key_exchange, CipherSuite2 *cipher_suite);
Handshake *ServerKeyExchangeToHandshake(ServerKeyExchange *client_server_key_exchange, CipherSuite2 *cipher_suite);
Handshake *FinishedToHandshake(Finished *finished);

// hanshake -> message
HelloRequest *HandshakeToHelloRequest(Handshake *handshake);
ClientServerHello *HandshakeToClientServerHello(Handshake *handshake);
Certificate *HandshakeToCertificate(Handshake *handshake);
CertificateRequest *HandshakeToCertificateRequest(Handshake *handshake);
ServerDone *HandshakeToServerdone(Handshake *handshake);
CertificateVerify *HandshakeToCertificateVerify(Handshake *handshake);
ClientKeyExchange *HandshakeToClientKeyExchange(Handshake *handshake, CipherSuite2 *cipher_suite);
ServerKeyExchange *HandshakeToServerKeyExchange(Handshake *handshake, CipherSuite2 *cipher_suite);
Finished *HandshakeToFinished(Handshake *handshake);

// record -> handshake
Handshake *RecordToHandshake(RecordLayer *record);

//handshake -> record
RecordLayer *HandshakeToRecordLayer(Handshake *handshake);

//change cipher spec protocol
RecordLayer *ChangeCipherSpecRecord();

/* CIPHERSUITE */
void setPriorities(uint8_t *number, uint8_t *priority, char *filename);
uint8_t *loadCipher(char* filename , uint8_t *len);
CipherSuite2 *CodeToCipherSuite(uint8_t ciphersuite_code);
uint8_t chooseChipher(ClientServerHello *client_supported_list, char *filename);


/* FREE FUNCTIONS */

void FreeRecordLayer(RecordLayer *recordLayer);

void FreeHandshake(Handshake *handshake);

void FreeClientServerHello(ClientServerHello *client_server_hello);
void FreeCertificate(Certificate *certificate);
void FreeCertificateVerify(CertificateVerify *certificate_verify);
void FreeServerHelloDone(ServerDone *server_done);
void FreeCertificateFinished(Finished *finished);
void FreeClientKeyExchange(ClientKeyExchange *client_server_key_exchange);
void FreeServerKeyExchange(ServerKeyExchange *client_server_key_exchange);

/* CERTIFICATE */
Certificate* loadCertificate(char * cert_name);
int writeCertificate(X509* certificate);
EVP_PKEY* readCertificateParam (Certificate *certificate);

/* KEY BLOCK*/

uint8_t *BaseFunction(int numer_of_MD5, uint8_t* principal_argument, int principal_argument_size, ClientServerHello *client_hello, ClientServerHello *server_hello);

uint8_t *MasterSecretGen(uint8_t *pre_master_secret, ClientServerHello *client_hello, ClientServerHello *server_hello);

uint8_t *KeyBlockGen(uint8_t *master_secret, CipherSuite2 *cipher_suite, ClientServerHello *client_hello, ClientServerHello *server_hello);

/* ENCRYPTION */

//asymmetric
uint8_t* encryptPreMaster(EVP_PKEY *pKey, KeyExchangeAlgorithm algorithm, uint8_t* pre_master_secret, int in_size, int *out_size);
uint8_t* decryptPreMaster(KeyExchangeAlgorithm alg, uint8_t *enc_pre_master_secret, int in_size, int *out_size);

//symmetric
uint8_t* DecEncryptPacket(uint8_t *in_packet, int in_packet_len, int *out_packet_len, CipherSuite2 *cipher_suite, uint8_t* key_block, Talker key_talker, int state);

/* AUTHENTICATION */
uint8_t* MAC(CipherSuite2 cipher, Handshake *hand, uint8_t* macWriteSecret);

