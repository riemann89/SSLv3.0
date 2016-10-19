#include "SSL_functions.h"
#include <openssl/md5.h>

/*****************************************FUNCTIONS***********************************************/

//CHANNEL FUNCTIONS

/*
 It allows the communication to the indicated talker: (0 - client, 1 - server, as defined in Talker enum)
 */

/**
 * Gives to talker the right to write on the main channel,
 * It writes an ID on the file token.txt
 * @param Talker talker
 */
void OpenCommunication(Talker talker){
    //VARIABLE DECLARATION//
    FILE* token;

    //CHECKING INPUT//
    if (talker!=client && talker!=server) {
        perror("Error in OpenCommunication -  Error in talker input (nor client, nor server input)");
        exit(1);
    }
    //AUTHORIZING SELECTED TALKER//
    token=fopen("token.txt", "w");
    if(token == NULL) {
        perror("Errore in apertura del file");
        exit(1);
    }
    fprintf(token,"%u",talker);
    fclose(token);
}

/**
 * It checks who between server/client can communicate. Return the rightful talker.
 * @return Talker authorized_talker
 */
Talker CheckCommunication(){
    
    //VARIABLES DECLARATION
    FILE* token;
    Talker authorized_talker;																													//returning variable
    token = fopen("token.txt", "r");    																										//on token.txt is saved the current authorized talker 
    if(token == NULL) {
        perror("Failed to open token.txt - CheckCommunication() operation\n");							//is the file empty?? it shouldn't
        exit(1);
    }
	//SEEK WICH ONE IS AUTHORIZETD TO TALK//
    fscanf(token,"%u",&(authorized_talker));																						//read value from talker and insert it into the returning variable
    fclose(token);																																		//Close file
    if (authorized_talker!=client && authorized_talker!=server) {
        perror("Error in token.txt - nor client,nor server authorized\n");													//is the read value an acceptable value?? in case not return an error.
        exit(1);
    }
    return authorized_talker;
}


/**
 * This function loads a certificate from a file and return an array of bites 
 * where are contained certificate information in DER format
 * @param char *cert_name
 * @return Certificate *certificate
 */
Certificate* loadCertificate(char * cert_name){
    
    Certificate *certificate;
    certificate = calloc(1,sizeof(certificate));
    X509* certificate_x509 = NULL;
    uint8_t *buf;
    FILE* certificate_file;
    
    int len = 0;
    
    buf = NULL;
    certificate_file = fopen(cert_name, "r");
    
    if (certificate_file == NULL){
        perror("Certificate File not found\n");
        exit(1);
    }
    
    certificate_x509 = PEM_read_X509(certificate_file, NULL, 0, NULL);
    len = i2d_X509(certificate_x509, &buf);
    
    certificate->X509_der = buf;
    certificate->len = len;
    return certificate;
}

/**
 * writes a record on SSLchannel.txt file
 * @param RecordLayer *record_layer
 */
void sendPacketByte(RecordLayer *record_layer){
	
    //Variables Declarations//
    FILE* SSLchannel;
    uint8_t length16[4],*message,*length,*Mversion,*mversion;
    ContentType *type;
    int_To_Bytes(record_layer->length, length16);																		//int to bytes representation of the lenght
    //Channel Operations//
    SSLchannel=fopen("SSLchannelbyte.txt", "wb");															 		//channel opening in creating-writing mode
    if (SSLchannel == NULL) {
        perror("Failed to open SSLchannel.txt - sendPacket operation\n");										//error
        exit(1);
    }
    //extracting fields from record_layer, loading into temporary variables
    type=&record_layer->type;
    length=&length16[2];
    message=record_layer->message;
    Mversion=&record_layer->version.major;
    mversion=&record_layer->version.minor;
    //record_layer fields serializing phase equivalent to transmit  those on the channel, in this toy our channel is just a file named SSLChannelbyte.txt
    fwrite(type,sizeof(uint8_t),sizeof(uint8_t),SSLchannel);
    fwrite(Mversion,sizeof(uint8_t),1,SSLchannel);
    fwrite(mversion,sizeof(uint8_t),1,SSLchannel);
    fwrite(length,sizeof(uint8_t),2,SSLchannel);
    for (int i=0; i<(record_layer->length-5); i++) {
        fwrite((message+i),sizeof(uint8_t),1,SSLchannel);
    }
    //channel closure
    fclose(SSLchannel);
}


/**
 * Read the file SSLchannelbyte.txt and parse it into a record layer structure
 * @return RecordLayer *returning_record
 */
RecordLayer  *readchannel(){
    uint8_t *buffer;
    uint8_t record_header[5];//rivedere
    FILE* SSLchannel;
    uint16_t packet_size;
    RecordLayer *returning_record;
    ContentType type;
    ProtocolVersion version;
    
    SSLchannel = fopen("SSLchannelbyte.txt", "rb");
    if(SSLchannel==NULL)
    {
        printf("Error unable to read the SSLchannel\n");
        exit(1);
    }
    
    fread(record_header, sizeof(uint8_t), 5*sizeof(uint8_t), SSLchannel);
    packet_size = Bytes_To_Int(2, record_header + 3);
    
    //buffer = (uint8_t*)malloc((packet_size - 5)*sizeof(uint8_t)); //alloc enough memory to handle SSLchannel file
    buffer = (uint8_t*)calloc(packet_size - 5, sizeof(uint8_t));
    fseek(SSLchannel, SEEK_SET, 5);
    fread(buffer,sizeof(uint8_t),(packet_size-5)*sizeof(uint8_t), SSLchannel);// load file into buffer
    
    returning_record = calloc(6, sizeof(uint8_t));
    
    type = record_header[0];
    returning_record->type = type;	// assign type
    
    version.minor = record_header[1]; //loading version
    version.major = record_header[2];
    returning_record->version = version;// assign version
    returning_record->length = packet_size;// assign length to record
    returning_record->message= buffer;
    return returning_record;//assign pointer to message
}

/***************************************FREE FUNCTIONS**********************************************/
/**
 * free memory allocated by recordLayer
 * @param *recordLayer
 */
void FreeRecordLayer(RecordLayer *recordLayer){
    free(recordLayer->message);
    free(recordLayer);
}

/**
 * free memory allocated by hello_request
 * @param *hello_request
 */
void FreeHelloRequest(HelloRequest *hello_request){
    free(hello_request);
}

/**
 * free memory allocated by handshake
 * @param *handshake
 */
void FreeHandshake(Handshake *handshake){
    free(handshake->content);
    free(handshake);
}

/**
 * free memory allocated by client_server_hello
 * @param *client_server_hello
 */
void FreeClientServerHello(ClientServerHello *client_server_hello){
    free(client_server_hello->ciphersuite);//TODO Rivedere
    free(client_server_hello->random);
    free(client_server_hello);
}

/**
 * free memory allocated by certificate
 * @param *certificate
 */
void FreeCertificate(Certificate *certificate){
    free((uint8_t *)certificate->X509_der);
    free(certificate);
}

/**
 * free memory allocated by certificat_request
 * @param *certificate_request
 */
void FreeCertificateRequest(CertificateRequest *certificate_request){
    free(certificate_request->certificate_authorities);
    free(certificate_request);
}

/**
 * free memory allocated by server_done
 * @param *server_done
 */
void FreeServerDone(ServerDone *server_done){
    free(server_done);
}

/**
 * free memory allocated by certificate_verify
 * @param *certificate_verify
 */
void FreeCertificateVerify(CertificateVerify *certificate_verify){
    free(certificate_verify->signature);
    free(certificate_verify);
}

/**
 * free memory allocated by server_key_exchange
 * @param *client_key_exchange
 */
void FreeClientKeyExchange(ClientKeyExchange *client_server_key_exchange){
    free(client_server_key_exchange->parameters);
    free(client_server_key_exchange);
}

/**
 * free memory allocated by server_key_exchange
 * @param *server_key_exchange
 */
void FreeServerKeyExchange(ServerKeyExchange *client_server_key_exchange){
    free(client_server_key_exchange->parameters);
    free(client_server_key_exchange->signature);
    free(client_server_key_exchange);
}

/**
 * free memory allocated by finished
 * @param *finished
 */
void FreeCertificateFinished(Finished *finished){
    // free(finished->hash);            VA LIBERATO??
    free(finished);  
}

/********************FUNCTION TO CONSTRUCT HANDSHAKE PROTOCOL MESSAGE TYPES*************************/
/* Message types to Handshake */

/**
 * creates a handshake wich contains a hellorequest
 * @return handshake
 */
Handshake *HelloRequestToHandshake(){
    //VARIABLE DECLARATION//
    Handshake *handshake;
    uint8_t* Bytes;
    
    //MEMORY ALLOCATION//
    Bytes = NULL;
    handshake=(Handshake*)calloc(1, sizeof(handshake));
    if (handshake == NULL) {
        perror("Failed to create handshake pointer - ServerDoneToHandshake operation");
        exit(1);
    }
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = HELLO_REQUEST;//handshake fields initialization
    handshake->length = 5;
    handshake->content = Bytes;
    return handshake;
}

/**
 * Serialize client_server_hello into handshake 
 * @param ClientServerHello *client_server_hello
 * @return Handshake *handshake
 */
Handshake *ClientServerHelloToHandshake(ClientServerHello *client_server_hello){
    //VARIABLE DECLARATION//
    CipherSuite *cipher;
    Handshake *handshake; 																		 	//returning variable
    uint8_t timeB[4]; 																					//current time bytes representation
    uint8_t session[4];			 																		//session bytes representation
    uint8_t cipher_codes[client_server_hello->length-38]; // 38= Random(32)+session(4) + version(1) + length(1)
    														//array of all ciphers supported
    uint8_t *Bytes;							//Used to serialize various fields of ClientServerHello and then pass to Handshake->content field
    
    //MEMORY ALLOCATION//
    Bytes =(uint8_t*)calloc(client_server_hello->length,sizeof(uint8_t));
    if (Bytes == NULL) {
        perror("Failed to create Bytes pointer - ClientServerHelloToHandshake operation");
        exit(1);
    }
    handshake=(Handshake*)calloc(1,sizeof(handshake));								//handshake memory allocation
    if (handshake == NULL) {
        perror("Failed to create handshake pointer - ClientServerHelloToHandshake operation");
        exit(1);
    }
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    cipher = client_server_hello->ciphersuite;   //temporary vector containing all cipher codes - it is requested to perform following memcopy
    for (int i=0;i<(client_server_hello->length-38);i++){  
        cipher_codes[i]=(cipher+i)->code;
    }
    int_To_Bytes(client_server_hello->random->gmt_unix_time, timeB);   	    //unix_time 
    int_To_Bytes(client_server_hello->sessionId, session);  								// session values to bytes transformation
    Bytes[0]=client_server_hello->version;   														//serializing client/server_hello field into bytes data vector
    memcpy(Bytes+1 ,session, 4);
    memcpy(Bytes+5 ,timeB , 4);
    memcpy(Bytes+9, client_server_hello->random->random_bytes,28);
    memcpy(Bytes+37, cipher_codes,client_server_hello->length-38);       		//38= version(1)+length(1)+session(4)+random(32)
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = client_server_hello->type;   												//handshake fields initialization
    handshake->length = client_server_hello->length + 3;
    handshake->content = Bytes;
    return handshake;
}

/**
 * Serialize certificate into handshake
 * @param Certificate *certificate
 * @return Handshake *handshake
 */
Handshake *CertificateToHandshake(Certificate *certificate){
    //VARIABLE DECLARATION//
    Handshake *handshake; 																		 	//returning variable
    //session bytes representation
    uint8_t *Bytes;																								//Used to serialize various fields of ClientServerHello and then pass to Handshake->content field
    //MEMORY ALLOCATION//
    Bytes =(uint8_t*)calloc(certificate->len, sizeof(uint8_t));																								 //bytes data vector, as said Bytes is an array which represents client_server_hello
    if (Bytes == NULL) {
        perror("Failed to create Bytes pointer - ClientServerHelloToHandshake operation");
        exit(1);
    }
    handshake=(Handshake*)calloc(1, sizeof(handshake));								//handshake memory allocation
    if (handshake == NULL) {
        perror("Failed to create handshake pointer - ClientServerHelloToHandshake operation");
        exit(1);
    }
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    memcpy(Bytes, certificate->X509_der, certificate->len);       		//38= version(1)+length(1)+session(4)+random(32)
    
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = CERTIFICATE;   												//handshake fields initialization
    handshake->length = certificate->len + 4;
    handshake->content = Bytes;
    return handshake;
}

/**
 * Serialize client_key_exchange into handshake
 * @param ClientKeyExchange *client_key_exchange
 * @return Handshake *handshake
 */
Handshake *ClientKeyExchangeToHandshake(ClientKeyExchange *client_server_key_exchange, CipherSuite2 *cipher_suite){
    Handshake *handshake;
    uint8_t *Bytes;

    
    Bytes = (uint8_t*)calloc(client_server_key_exchange->len_parameters, sizeof(uint8_t));
    if (Bytes == NULL) {
        perror("Failed to create Bytes pointer - ClientKeyExchangeToHandshake operation");
        exit(1);
    }
    
    handshake=(Handshake*)calloc(1,sizeof(handshake));
    if (handshake == NULL) {
        perror("Failed to create handshake pointer - ClientKeyToHandshake operation");
        exit(1);
    }
    
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    memcpy(Bytes, client_server_key_exchange->parameters, client_server_key_exchange->len_parameters);
    
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = CLIENT_KEY_EXCHANGE;
    handshake->length = 4 + client_server_key_exchange->len_parameters;
    handshake->content = Bytes;
    
    return handshake;
}
Handshake *ServerKeyExchangeToHandshake(ServerKeyExchange *client_server_key_exchange, CipherSuite2 *cipher_suite){
    Handshake *handshake;
    uint8_t *Bytes;
    
    
    Bytes = (uint8_t*)calloc(client_server_key_exchange->len_parameters + cipher_suite->signature_size, sizeof(uint8_t));
    if (Bytes == NULL) {
        perror("Failed to create Bytes pointer - ClientKeyExchangeToHandshake operation");
        exit(1);
    }
    
    handshake=(Handshake*)calloc(1,sizeof(handshake));
    if (handshake == NULL) {
        perror("Failed to create handshake pointer - ClientKeyToHandshake operation");
        exit(1);
    }
    
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    memcpy(Bytes, client_server_key_exchange->parameters, client_server_key_exchange->len_parameters);
    memcpy(Bytes + client_server_key_exchange->len_parameters, client_server_key_exchange->signature, cipher_suite->signature_size);
    
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = CLIENT_KEY_EXCHANGE;
    handshake->length = 4 + client_server_key_exchange->len_parameters + cipher_suite->signature_size;
    handshake->content = Bytes;
    
    return handshake;
}//TOCHECK

/**
 * Serialize certificate_request into handshake
 * @param CertificateRequest *certificate_request
 * @return Handshake *handshake
 */
Handshake *CertificateRequestToHandshake(CertificateRequest *certificate_request){
    //VARIABLE DECLARATION//
    Handshake *handshake;
    uint8_t *Bytes;
    int bytes_size;
    
    bytes_size = certificate_request->list_length * certificate_request->name_lenght + 1;
    
    //MEMORY ALLOCATION//
    Bytes =(uint8_t*)calloc(bytes_size, sizeof(uint8_t));
    
    if (Bytes == NULL) {
        perror("Failed to create Bytes pointer - ClientServerHelloToHandshake operation");
        exit(1);
    }
    handshake=(Handshake*)calloc(1, sizeof(handshake));
    if (handshake == NULL) {
        perror("Failed to create handshake pointer - ClientServerHelloToHandshake operation");
        exit(1);
    }
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    Bytes[0] = certificate_request->certificate_type;
    memcpy(Bytes + 1, certificate_request->certificate_authorities, bytes_size);
    
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = CERTIFICATE_REQUEST;   											//handshake fields initialization
    handshake->length = 4 + bytes_size;
    handshake->content = Bytes;
    return handshake;
}

Handshake *ServerDoneToHandshake(){
    //VARIABLE DECLARATION//
    Handshake *handshake;
    uint8_t* Bytes;
    
    //MEMORY ALLOCATION//
    Bytes = NULL;
    handshake=(Handshake*)calloc(1, sizeof(handshake));
    if (handshake == NULL) {
        perror("Failed to create handshake pointer - ServerDoneToHandshake operation");
        exit(1);
    }
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = SERVER_DONE;//handshake fields initialization
    handshake->length = 4;
    handshake->content = Bytes;
    return handshake;
}

/**
 * Serialize certificate_verify into handshake
 * @param CertificateVerify *certificate_verify
 * @return Handshake *handshake
 */
Handshake *CertificateVerifyToHandshake(CertificateVerify *certificate_verify){
    Handshake *handshake;
    uint8_t *Bytes;
    int bytes_size;
    
    bytes_size = 0;
    Bytes = NULL;
    
    switch (certificate_verify->algorithm_type) {
        case SHA1_:
            bytes_size = 20;
            break;
        case MD5_1:
            bytes_size = 16;
            break;
        default:
            perror("CertificateVerifyToHandshake error: algorithm type not recognized.");
            break;
    }
    
    Bytes = (uint8_t*)calloc(bytes_size, sizeof(uint8_t));
    if (Bytes == NULL) {
        perror("Failed to create Bytes pointer - FinishedToHandshake operation");
        exit(1);
    }
    handshake=(Handshake*)calloc(1,sizeof(handshake));
    if (handshake == NULL) {
        perror("Failed to create handshake pointer - FinishedToHandshake operation");
        exit(1);
    }
    
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    Bytes[0] = certificate_verify->algorithm_type;
    memcpy(Bytes + 1, certificate_verify->signature, bytes_size);
    
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = CERTIFICATE_VERIFY;
    handshake->length = 4 + bytes_size + 1;
    handshake->content = Bytes;
    
    return handshake;
}

/**
 * Serialize finished into handshake
 * @param Finished *finished
 * @return Handshake *handshake
 */
Handshake *FinishedToHandshake(Finished *finished){
    Handshake *handshake;
    uint8_t *Bytes;
    
    Bytes = (uint8_t*)calloc(36, sizeof(uint8_t));
    if (Bytes == NULL) {
        perror("ERROR FinishedToHandshake: Failed to create Bytes pointer");
        exit(1);
    }
    handshake=(Handshake*)calloc(1, sizeof(handshake));
    if (handshake == NULL) {
        perror("ERROR FinishedToHandshake: Failed to create Handshake pointer");
        exit(1);
    }
    
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    
    memcpy(Bytes, finished->hash, 36); //MD5 + SHA1
    
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = FINISHED;
    handshake->length = 4 + 36;
    handshake->content = Bytes;
    
    return handshake;
}

/********************FUNCTION TO CONSTRUCT PACKET FROM HANDSHAKE*************************/

/**
 * Parse handshake into hellorequest
 * @param Handshake *handshake
 * @return HelloRequest *hello_request
 */
HelloRequest *HandshakeToHelloRequest(Handshake *handshake){
    HelloRequest *hello_request;
    
    if (handshake->msg_type != HELLO_REQUEST){
            perror("ERROR HandshakeToHelloRequest: handshake does not contain an hello request message.");
            exit(1);
        }
    
    hello_request = (HelloRequest*)calloc(1, sizeof(HelloRequest));
  
    
    
    if (hello_request == NULL){
        perror("ERROR HandshakeToHelloRequest: memory allocation leak.");
        exit(1);
    }
    
    return hello_request;
    }

/**
 *  Parse handshake into client_server_hello
 * @param Handshake handshake
 * @return ClientServerHello client_server_hello
 */
ClientServerHello *HandshakeToClientServerHello(Handshake *handshake){
    ClientServerHello *client_server_hello;
    CipherSuite *ciphers;
    Random *random;
    ciphers = (CipherSuite*)calloc(handshake->length-41,sizeof(CipherSuite));    
    client_server_hello = (ClientServerHello*)calloc(1, sizeof(ClientServerHello));
    random = (Random*)calloc(1,sizeof(Random));
    
    random->gmt_unix_time = Bytes_To_Int(4, handshake->content + 5);
    
    if (handshake->msg_type != CLIENT_HELLO && handshake->msg_type != SERVER_HELLO){
        printf("%d\n",handshake->msg_type);
        perror("HandshakeToClientServerHello: handshake does not contain a client_hello/server_hello message.\n");
        exit(1);
    }
    
    memcpy(random->random_bytes, handshake->content + 9,28);
    memcpy(ciphers, handshake->content + 37, (handshake->length-41));
        
    client_server_hello->length = handshake->length-4;
    client_server_hello->version = handshake->content[0];
    client_server_hello->sessionId = Bytes_To_Int(4, handshake->content + 1);
    client_server_hello->random = random;

    client_server_hello->ciphersuite = ciphers;

    return client_server_hello;
}//RIVEDERE da completare

/**
 *  Parse handshake into certificate
 * @param Handshake *handshake
 * @return Certificate *certificate
 */
Certificate *HandshakeToCertificate(Handshake *handshake){
    Certificate *certificate;
    uint8_t *buffer;
    int certificate_len;
    
    if (handshake->msg_type != CERTIFICATE){
        perror("ERROR HandshakeToCertificate: handshake does not contain a certificate message.");
        exit(1);
    }
    
    certificate_len = handshake->length - 4;
    
    certificate = (Certificate *)calloc(1, sizeof(Certificate));
    
    if (certificate == NULL){
        perror("ERROR HandshakeToHelloRequest: memory allocation leak.");
        exit(1);
    }
    
    buffer = (uint8_t *)calloc(certificate_len, sizeof(uint8_t));
    
    memcpy(buffer, handshake->content, certificate_len);
    
    certificate->len = certificate_len;
    certificate->X509_der = buffer;
    
    return certificate;
}

/**
 *  Parse handshake into server_done
 * @param Handshake *handshake
 * @return ServerDone *server_done
 */
ServerDone *HandshakeToServerdone(Handshake *handshake){
    ServerDone *server_done;
    
    if (handshake->msg_type != SERVER_DONE){
        perror("ERROR HandshakeToServerDone: handshake does not contain a server done message.");
        exit(1);
    }
    
    server_done = (ServerDone*)calloc(1, sizeof(ServerDone));
    
    if (server_done == NULL){
        perror("ERROR HandshakeToServerDone: memory allocation leak.");
        exit(1);
    }
    
    return server_done;

};//TOCHECK

/**
 *  Parse handshake into certificate_verify
 * @param Handshake *handshake
 * @return CertificateVerify certificate_verify
 */
CertificateVerify *HandshakeToCertificateVerify(Handshake *handshake){
    CertificateVerify *certificate_verify;
    uint8_t *signature;
    int signature_len;
    
    if (handshake->msg_type != CERTIFICATE_VERIFY){
        perror("ERROR HandshakeToCertificateVerify: handshake does not contain a certificate verify message.");
        exit(1);
    }
    
    certificate_verify = (CertificateVerify *)calloc(1, sizeof(CertificateVerify));
    if (certificate_verify == NULL){
        perror("ERROR HandshakeToCertificateVerify: memory allocation leak.");
        exit(1);
    }
    
    signature_len = handshake->length - 4;
    signature = (uint8_t *)calloc(signature_len, sizeof(uint8_t));
    if (signature == NULL){
        perror("ERROR HandshakeToCertificateVerify: memory allocation leak.");
        exit(1);
    }
    
    switch (signature_len) {
        case 20:
            certificate_verify->algorithm_type = SHA1_;
            break;
            
        case 16:
            certificate_verify->algorithm_type = MD5_1;
            break;
        default:
            perror("ERROR HandshakeToCertificateVerify: signature size not valid.");
            exit(1);
            
    }
    
    memcpy(signature, handshake->content, signature_len);
    
    return certificate_verify;
    
}//TOCHECK

/**
 *  Parse handshake into server_key_exchange
 * @param Handshake *handshake
 * @param KeyExchangeAlgorithm algorithm_type
 * @param SignatureAlgorithm signature_type
 * @param uint32_t len_parameters
 * @param uint32_t len_signature
 * @return ServerKeyExchange *server_key_exchange
 */

ClientKeyExchange *HandshakeToClientKeyExchange(Handshake *handshake, CipherSuite2 *cipher_suite){
    
    ClientKeyExchange *client_server_key_exchange;
    
    if (handshake->msg_type != CLIENT_KEY_EXCHANGE){
        perror("ERROR HandshakeToClientKeyExchange: handshake does not contain a client key message.");
        exit(1);
    }
    
    client_server_key_exchange = (ClientKeyExchange *)calloc(1, sizeof(ClientKeyExchange));
    if (client_server_key_exchange == NULL){
        perror("ERROR HandshakeToClientKeyExchange: memory allocation leak.");
        exit(1);
    }
    
    
    client_server_key_exchange->len_parameters = handshake->length - 4;
    
    client_server_key_exchange->parameters = (uint8_t *)calloc(client_server_key_exchange->len_parameters, sizeof(uint8_t));
    
    if (client_server_key_exchange->parameters == NULL){
        perror("ERROR HandshakeToClientKeyExchange: memory allocation leak.");
        exit(1);
    }
    
    
    memcpy(client_server_key_exchange->parameters, handshake->content, client_server_key_exchange->len_parameters);
    
    return client_server_key_exchange;
}

ServerKeyExchange *HandshakeToServerKeyExchange(Handshake *handshake, CipherSuite2 *cipher_suite){
    
    ServerKeyExchange *client_server_key_exchange;
    
    if (handshake->msg_type != CLIENT_KEY_EXCHANGE){
        perror("ERROR HandshakeToClientKeyExchange: handshake does not contain a client key message.");
        exit(1);
    }
    
    client_server_key_exchange = (ServerKeyExchange *)calloc(1, sizeof(ServerKeyExchange));
    if (client_server_key_exchange == NULL){
        perror("ERROR HandshakeToClientKeyExchange: memory allocation leak.");
        exit(1);
    }
    
    
    client_server_key_exchange->len_parameters = handshake->length - 5 - cipher_suite->signature_size;
    
    client_server_key_exchange->parameters = (uint8_t *)calloc(client_server_key_exchange->len_parameters, sizeof(uint8_t));
    
    if (client_server_key_exchange->parameters == NULL){
        perror("ERROR HandshakeToClientKeyExchange: memory allocation leak.");
        exit(1);
    }
    
    client_server_key_exchange->signature = (uint8_t *)calloc(cipher_suite->signature_size, sizeof(uint8_t));
    
    if (client_server_key_exchange->signature == NULL){
        perror("ERROR HandshakeToClientKeyExchange: memory allocation leak.");
        exit(1);
    }
    
    memcpy(handshake->content, client_server_key_exchange->parameters, client_server_key_exchange->len_parameters);
    memcpy(handshake->content + client_server_key_exchange->len_parameters, client_server_key_exchange->signature, cipher_suite->signature_size);
    
    return client_server_key_exchange;
}//TOCHECK

/**
 *  Parse handshake into finished
 * @param Handshake *handshake
 * @return Finished *finished
 */
Finished *HandshakeToFinished(Handshake *handshake){
    Finished *finished;
   
    if (handshake->msg_type != FINISHED){
        perror("ERROR HandshakeToFinished: handshake does not contain a finished message.");
        exit(1);
    }
    
    finished = (Finished *)calloc(1, sizeof(Finished));
    if (finished == NULL){
        perror("ERROR HandshakeToFinished: memory allocation leak.");
        exit(1);
    }
    
    memcpy(finished->hash, handshake->content, 36);
    
    return finished;


}//TOCHECK

/**
 *  Parse handshake into certificate_request
 * @param Handshake *handshake
 * @return CertificateRequest *certificate_request
 */
CertificateRequest *HandshakeToCertificateRequest(Handshake *handshake){
 CertificateRequest *certificate_request;
 uint8_t *buffer;
 int buffer_len;
 
 if (handshake->msg_type != CERTIFICATE_REQUEST){
 perror("ERROR HandshakeToCertificateRequest: handshake does not contain a certificate request message.");
 exit(1);
 }
 
 buffer_len = handshake->length - 4;
 
 certificate_request = (CertificateRequest *)calloc(1, sizeof(CertificateRequest));
 if (certificate_request == NULL){
 perror("ERROR HandshakeToCertificateRequest: memory allocation leak.");
 exit(1);
 }
 
 buffer = (uint8_t *)calloc(buffer_len, sizeof(uint8_t));
 if (buffer == NULL){
 perror("ERROR HandshakeToCertificateRequest: memory allocation leak.");
 exit(1);
 }
 
 memcpy(buffer, handshake->content, buffer_len);
 
 //certificate->len = certificate_len;
 //certificate->X509_der = buffer;
 
 return certificate_request;
 } //TODO Rivedere: non riesco a rappresentare le liste, rivedere anche la struttura certificate_request.

/***************************************HANDSHAKE TO/FROM RECORDLAYER******************************************************/

/**
 * Serialize handshake into record_layer
 * @param Handshake *handshake
 * @return RecordLayer *recordlayer
 */
RecordLayer *HandshakeToRecordLayer(Handshake *handshake){
    //VARIABLE DECLARATION//
    uint8_t *Bytes;
    uint8_t length24[4];
    RecordLayer *recordlayer;  																										//returning variable
    int len;
    //MEMORY ALLOCATION//
    Bytes =(uint8_t*)calloc(handshake->length,sizeof(uint8_t)); 			    								//bytes data vector allocation
    if (Bytes == NULL) { 																													//contain the lenght of corresponding vector
        perror("Failed to create Bytes pointer - HandshakeToRecordLayer operation");
        exit(1);
    }
    recordlayer = (RecordLayer*)calloc(1,sizeof(RecordLayer));																									//record layer allocation memory i need 5 extra-bytes  
    if (recordlayer == NULL) {
        perror("Failed to create recordlayer pointer - HandshakeToRecordLayer operation");
        exit(1);
    }
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    int_To_Bytes(handshake->length ,length24); 			  				  												
    len=handshake->length;							
    Bytes[0]=handshake->msg_type;
    memcpy(Bytes+1 ,length24+1,3);
    memcpy(Bytes+ 4 ,handshake->content,len-4);
	//RECORDLAYER CONSTRUCTION//
    recordlayer->type=HANDSHAKE;
    recordlayer->version=std_version;
    recordlayer->length=handshake->length+5;
    recordlayer->message=Bytes;
    return recordlayer;
}

RecordLayer *ChangeCipherSpecRecord(){
    
    RecordLayer *recordlayer;
    uint8_t *byte;
    byte =(uint8_t*)calloc(1,sizeof(uint8_t)); 
    recordlayer = (RecordLayer*)calloc(1, sizeof(RecordLayer));
    
    byte[0]=1;
    
    recordlayer->type= CHANGE_CIPHER_SPEC;
    recordlayer->version= std_version;
    recordlayer->length= 6;
    recordlayer->message = byte;
    
    return recordlayer;   
}

/* RecordLayer to Handshake */
Handshake *RecordToHandshake(RecordLayer *record){
    Handshake *result;
    uint8_t *buffer;
    
    result = calloc(1, sizeof(Handshake));
    buffer = (uint8_t*)malloc((record->length - 9)*sizeof(uint8_t));
    
    if(record->type != HANDSHAKE){
        printf("\n RecordToHandshake: Error record is not a handshake,  parse failed");
        exit(1);
        return NULL;
    }
    
    memcpy(buffer,  record->message + 4, record->length - 9);
    result->length = record->length - 5;
    result->msg_type = record->message[0];
    result->content = buffer;

    return result;
    
}

/**
 * write on the file named *filename the list of chiphersuite *priority coded as uint8_t which length is *number,
 * the list *priority should be sorted in decrescent order of priority.
 * @param uint8_t *number
 * @param uint8_t *priority
 * @param char *filename
 */
void setPriorities(uint8_t *number, uint8_t *priority, char *filename){   															

    FILE* PriorityList; 																														
    PriorityList = fopen(filename , "wb");   																																													 	
    fwrite(number,sizeof(uint8_t),1,PriorityList);
   
    for(int i = 0; i<*number; i++){   																									
        fwrite(&(priority +i),sizeof(uint8_t),1,PriorityList);
    }
    fclose(PriorityList);                                                                                                                    
}

//this function read from PryorityList.txt and the input struct ClientServerHello, comparing chiphers Priority and avaiable and choosing the best fitting in a naive way

/**
 * compare the client_supported_list of ciphersuite containded in ClientHello with the ones contained in the *filename,
 * which is the file whose content is the list of chipher supported by server.
 * Both list should be set in decrescent order of priority to choose the best possible one.  
 * @param ClientServerHello *client_supported_list
 * @param char *filename
 * @return uint8_t chosenChipher
 */
uint8_t chooseChipher(ClientServerHello *client_supported_list, char *filename){
    FILE* PriorityList;
    uint8_t choosen;
    PriorityList = fopen(filename, "rb");  	 																		
    uint8_t *buffer;
    buffer = (uint8_t *)malloc((32)*sizeof(uint8_t));
    fread(buffer, 32, 1, PriorityList);
    //printf("%d\n",buffer[0]);
    for(int i=1; i<(int)(buffer[0])+1; i++){
        //printf("%d, %02X \n", i, buffer[i]);
        for(int j=0;j<client_supported_list->length -38 ;j++){ 
           //printf("    %d: %02X\n",j,client_supported_list->ciphersuite[j].code);
            if(buffer[i]==client_supported_list->ciphersuite_code[j]){  
                choosen = buffer[i];
                return choosen; 																									
            }
            
        }
    }
    printf("\nError, uncompatibles chiphers\n");
    exit(1);
}

/**
 * read from file a list of code of chiphersuites, the file should have a format 
 * first byte = number of the contained ciphersuite codes ,
 * following bytes = chiphersuite codes
 * @param char *filename
 * @param uint8_t *len
 * @return uint8_t *buffer 
 */
uint8_t *loadCipher(char* filename, uint8_t *len){
       
    FILE* CipherList;
    uint8_t *buffer;

    CipherList = fopen(filename, "rb");
    
   	if (CipherList == NULL){
        perror("loadCipher error: memory allocation leak.");
        exit(1);
    }
    
    fread(len, sizeof(uint8_t), 1, CipherList);
    buffer = (uint8_t *)malloc((len[0])*sizeof(uint8_t));
    fread(buffer, len[0]*sizeof(uint8_t), 1, CipherList);
    fclose(CipherList);
    
    return buffer;    
} 
/**
 * From the code of a ciphersuite decide if it match with RSA_, DIFFIE_HELLMAN or FORTEZZA.
 * @param uint8_t cipher_code
 * @return KeyExchangeAlgorithm 
 */
KeyExchangeAlgorithm getAlgorithm(uint8_t cipher_code){
    if(cipher_code> 0 && cipher_code < 11)
        return RSA_;
    if(cipher_code > 10 && cipher_code< 28)
        return DIFFIE_HELLMAN;
    if(cipher_code > 27 && cipher_code < 31)
        return KFORTEZZA;
    perror("Cipher null or not a valid \n");
    exit(1);
}

/*************************************** OTHERS ******************************************************/
CipherSuite2 *CodeToCipherSuite(uint8_t ciphersuite_code){
    //INIZIALIZZARE CipherSuite
    
    CipherSuite2 *cipher_suite;
    
    cipher_suite = (CipherSuite2*)calloc(1, sizeof(CipherSuite2));
    
    switch (ciphersuite_code) {
        case 0x00:
            cipher_suite->key_exchange_algorithm = KNULL;
            cipher_suite->cipher_type = TNULL;
            cipher_suite->cipher_algorithm = CNULL;
            cipher_suite->iv_size = 0;
            cipher_suite->key_material = 0;
            cipher_suite->signature_algorithm = SNULL;
            cipher_suite->exportable = false;
            cipher_suite->signature_size = 0;
            break;
            
        case 0x01:
            cipher_suite->key_exchange_algorithm = RSA_;
            cipher_suite->cipher_type = TNULL;
            cipher_suite->cipher_algorithm = CNULL;
            cipher_suite->iv_size = 0;
            cipher_suite->key_material = 0;
            cipher_suite->signature_algorithm = MD5_1;
            cipher_suite->exportable = false;
            cipher_suite->signature_size = 16;
            break;
            
        case 0x02:
            cipher_suite->key_exchange_algorithm = RSA_;
            cipher_suite->cipher_type = TNULL;
            cipher_suite->cipher_algorithm = CNULL;
            cipher_suite->iv_size = 0;
            cipher_suite->key_material = 0;
            cipher_suite->signature_algorithm = SHA1_;
            cipher_suite->exportable = false;
            cipher_suite->signature_size = 20;
            break;
            
        case 0x03:
            cipher_suite->key_exchange_algorithm = RSA_;
            cipher_suite->cipher_type = STREAM;
            cipher_suite->cipher_algorithm = RC4;
            cipher_suite->iv_size = 0;
            cipher_suite->key_material = 5;
            cipher_suite->signature_algorithm = MD5_1;
            cipher_suite->exportable = true;
            cipher_suite->signature_size = 16;
            break;
            
        case 0x04:
            cipher_suite->key_exchange_algorithm = RSA_;
        	cipher_suite->cipher_type = STREAM;
            cipher_suite->cipher_algorithm = RC4;
            cipher_suite->iv_size = 0;
            cipher_suite->key_material = 16;
            cipher_suite->signature_algorithm = MD5_1;
            cipher_suite->exportable = false;
            cipher_suite->signature_size = 16;
            break;
            
        case 0x05:
            cipher_suite->key_exchange_algorithm = RSA_;
            cipher_suite->cipher_type = STREAM;
            cipher_suite->cipher_algorithm = RC4;
            cipher_suite->iv_size = 0;
            cipher_suite->key_material = 16;
            cipher_suite->signature_algorithm = SHA1_;
            cipher_suite->exportable = false;
            cipher_suite->signature_size = 20;
            break;
            
        case 0x06:
            cipher_suite->key_exchange_algorithm = RSA_;
            cipher_suite->cipher_type = BLOCK;
            cipher_suite->cipher_algorithm = RC2;
            cipher_suite->iv_size = 8;
            cipher_suite->key_material = 5;
            cipher_suite->signature_algorithm = MD5_1;
            cipher_suite->exportable = true;
            cipher_suite->signature_size = 16;
            break;
            
        case 0x07:
            cipher_suite->key_exchange_algorithm = RSA_;
            cipher_suite->cipher_type = BLOCK;
            cipher_suite->cipher_algorithm = IDEA;
            cipher_suite->iv_size = 8;
            cipher_suite->key_material = 16;
            cipher_suite->signature_algorithm = SHA1_;
            cipher_suite->exportable = false;
            cipher_suite->signature_size = 20;
            break;
            
        case 0x08:
            cipher_suite->key_exchange_algorithm = RSA_;
            cipher_suite->cipher_type = BLOCK;
            cipher_suite->cipher_algorithm = DES40;
            cipher_suite->iv_size = 8;
            cipher_suite->key_material = 5;
            cipher_suite->signature_algorithm = SHA1_;
            cipher_suite->exportable = true;
            cipher_suite->signature_size = 20;
            break;
            
        case 0x09:
            cipher_suite->key_exchange_algorithm = RSA_;
            cipher_suite->cipher_type = BLOCK;
            cipher_suite->cipher_algorithm = DES;
            cipher_suite->iv_size = 8;
            cipher_suite->key_material = 8;
            cipher_suite->signature_algorithm = SHA1_;
            cipher_suite->exportable = false;
            cipher_suite->signature_size = 20;
            break;
            
        case 0x0A:
            cipher_suite->key_exchange_algorithm = RSA_;
            cipher_suite->cipher_type = BLOCK;
            cipher_suite->cipher_algorithm = DES3;
            cipher_suite->iv_size = 8;
            cipher_suite->key_material = 24;
            cipher_suite->signature_algorithm = SHA1_;
            cipher_suite->exportable = false;
            cipher_suite->signature_size = 20;
            break;
            
        case 0x0B:
            break;
            
        case 0x0C:
            break;
            
        case 0x0D:
            break;
        
        case 0x0E:
            break;
            
        case 0x0F:
            break;
            
        case 0x10:
            break;
            
        case 0x11:
            break;
            
        case 0x12:
            break;
            
        case 0x13:
            break;
            
        case 0x14:
            break;
            
        case 0x15:
            break;
            
        case 0x16:
            break;
            
        case 0x17:
            break;
            
        case 0x18:
            break;
            
        case 0x19:
            break;
            
        case 0x1A:
            break;
            
        case 0x1B:
            break;
            
        case 0x1C:
            break;
            
        case 0x1D:
            break;
        
        case 0x1E:
            break;
            
        default:
            perror("CodeToCipherSuite Error: code not valid");
            exit(1);
            break;
    }
    return cipher_suite;
	}

uint8_t *BaseFunction(int numer_of_MD5, uint8_t* principal_argument, int principal_argument_size, ClientServerHello *client_hello, ClientServerHello *server_hello){
    uint8_t *buffer;
    uint8_t letter;
    MD5_CTX md5;
    SHA_CTX sha;
    uint8_t *md5_1, *sha_1;
    
    letter = 65;
    
    buffer = calloc(16*numer_of_MD5, sizeof(uint8_t));
    
    if (buffer == NULL){
        perror("ERROR base_function: memory allocation leak.");
        exit(1);
    }
    
    sha_1 = calloc(20, sizeof(uint8_t));
    
    if (sha_1 == NULL){
        perror("ERROR base_function: memory allocation leak.");
        exit(1);
    }
    
    md5_1 = calloc(16, sizeof(uint8_t));
    
    if (md5_1 == NULL){
        perror("ERROR base_function: memory allocation leak.");
        exit(1);
    }
    
    for(int i = 0; i < numer_of_MD5 ; i++){
        SHA1_Init(&sha);
        letter = letter + i;
        
        for (int j = 0; j < i + 1; j++) {
            SHA_Update(&sha, &letter, sizeof(uint8_t));
        }
        
        SHA1_Update(&sha, principal_argument, principal_argument_size*sizeof(uint8_t));
        SHA1_Update(&sha, &client_hello->random->gmt_unix_time, sizeof(uint32_t));
        SHA1_Update(&sha, client_hello->random->random_bytes, 28*sizeof(uint8_t));
        SHA1_Update(&sha, &server_hello->random->gmt_unix_time, sizeof(uint32_t));
        SHA1_Update(&sha, server_hello->random->random_bytes, 28*sizeof(uint8_t));
        
        SHA1_Final(sha_1, &sha);
        
        MD5_Init(&md5);
        MD5_Update(&md5, principal_argument, principal_argument_size*sizeof(uint8_t));
        MD5_Update(&md5, sha_1, 20*sizeof(uint8_t));
        MD5_Final(md5_1, &md5);
        
        memcpy(buffer + 16*i, md5_1, 16*sizeof(uint8_t));
    }
    return buffer;
    
}

/*************************************** CERTIFICATES ******************************************************/
int writeCertificate(X509* certificate){
    /* Per leggere il der
    X509 *res= NULL;
    d2i_X509(&res, &buf, *len);
     */
    FILE* file_cert;
    
    file_cert=fopen("cert_out.crt", "w+");
    return PEM_write_X509(file_cert, certificate);
}
int readCertificate(){return 0;}

EVP_PKEY* readCertificateParam (Certificate *certificate){
    
    X509 *cert_509;
    EVP_PKEY *pubkey;
    
    cert_509 = NULL;
    
    cert_509 = d2i_X509(NULL, &(certificate->X509_der), certificate->len);
    
    if(cert_509 == NULL){
        perror("readCertificateParam Error: memory allocation leak.");
    }
    
    pubkey = X509_get_pubkey(cert_509);
    EVP_PKEY_id(pubkey);
    return pubkey;
}

/*************************************** KEYS GENERATION ******************************************************/
//Return size (in bytes) of keyblock

uint8_t *MasterSecretGen(uint8_t *pre_master_secret, ClientServerHello *client_hello, ClientServerHello *server_hello){
    uint8_t *master_secret;
    
    master_secret = BaseFunction(3, pre_master_secret, 48, client_hello, server_hello);
    
    if (master_secret == NULL) {
        perror("MasterSecretGen Error: memory allocation leak.");
        exit(1);
    }
    
    return master_secret;
}
uint8_t *KeyBlockGen(uint8_t *master_secret, CipherSuite2 *cipher_suite, ClientServerHello *client_hello, ClientServerHello *server_hello){
    
    uint8_t *key_block, *final_client_write_key, *final_server_write_key, *client_write_iv, *server_write_iv;
    MD5_CTX md5;
    int key_block_size, key_block_size_temp;
    
    key_block = NULL;
    final_client_write_key = NULL;
    final_server_write_key = NULL;
    key_block_size = 0;
    key_block_size_temp = 0;
    
    if (cipher_suite->exportable == false) {
        key_block_size = 2*(cipher_suite->signature_size + cipher_suite->key_material + cipher_suite->iv_size);
        key_block_size = key_block_size + key_block_size % 16; //made a multiple of 16
        key_block = BaseFunction(key_block_size/16, master_secret, 48, client_hello, server_hello);
    }
    else{
        //KeyBlock temp
        key_block_size_temp = 2*(cipher_suite->signature_size + cipher_suite->key_material);
        key_block_size_temp = key_block_size_temp + key_block_size_temp % 16; //made a multiple of 16
        key_block = BaseFunction(key_block_size_temp, master_secret, 48, client_hello, server_hello);
        
        //final write key
        //client
        final_client_write_key = calloc(16, sizeof(uint8_t));
        
    	MD5_Init(&md5);
        MD5_Update(&md5, key_block + 2*(cipher_suite->signature_size), cipher_suite->key_material);
        MD5_Update(&md5, &client_hello->random->gmt_unix_time, sizeof(uint32_t));
        MD5_Update(&md5, client_hello->random->random_bytes, 28*sizeof(uint8_t));
        MD5_Update(&md5, &server_hello->random->gmt_unix_time, sizeof(uint32_t));
        MD5_Update(&md5, server_hello->random->random_bytes, 28*sizeof(uint8_t));
        MD5_Final(final_client_write_key, &md5);
        
        //server
        final_server_write_key = calloc(16, sizeof(uint8_t));
        
        MD5_Init(&md5);
        MD5_Update(&md5, key_block + 2*(cipher_suite->signature_size) + cipher_suite->key_material, cipher_suite->key_material);
        MD5_Update(&md5, &server_hello->random->gmt_unix_time, sizeof(uint32_t));
        MD5_Update(&md5, server_hello->random->random_bytes, 28*sizeof(uint8_t));
        MD5_Update(&md5, &client_hello->random->gmt_unix_time, sizeof(uint32_t));
        MD5_Update(&md5, client_hello->random->random_bytes, 28*sizeof(uint8_t));
        MD5_Final(final_server_write_key, &md5);
        
        //iv bytes
        client_write_iv = calloc(16, sizeof(uint8_t));
        
        MD5_Init(&md5);
        MD5_Update(&md5, &client_hello->random->gmt_unix_time, sizeof(uint32_t));
        MD5_Update(&md5, client_hello->random->random_bytes, 28*sizeof(uint8_t));
        MD5_Update(&md5, &server_hello->random->gmt_unix_time, sizeof(uint32_t));
        MD5_Update(&md5, server_hello->random->random_bytes, 28*sizeof(uint8_t));
        MD5_Final(final_client_write_key, &md5);
        
        //server
        server_write_iv = calloc(16, sizeof(uint8_t));
        
        MD5_Init(&md5);
        MD5_Update(&md5, &server_hello->random->gmt_unix_time, sizeof(uint32_t));
        MD5_Update(&md5, server_hello->random->random_bytes, 28*sizeof(uint8_t));
        MD5_Update(&md5, &client_hello->random->gmt_unix_time, sizeof(uint32_t));
        MD5_Update(&md5, client_hello->random->random_bytes, 28*sizeof(uint8_t));
        MD5_Final(final_server_write_key, &md5);
        
        //construct final keyblock
        memcpy(key_block + 2*(cipher_suite->signature_size), final_client_write_key, 16);
        memcpy(key_block + 2*(cipher_suite->signature_size) + 16, final_server_write_key, 16);
        memcpy(key_block + 2*(cipher_suite->signature_size) + 32, final_client_write_key, 16);
        memcpy(key_block + 2*(cipher_suite->signature_size) + 48, final_server_write_key, 16);
    }
    
    
    
	
    return key_block;
    
    
}

/*************************************** ENCRYPTION ******************************************************/
//Asymmetric
uint8_t* encryptPreMaster(EVP_PKEY *pKey, KeyExchangeAlgorithm algorithm, uint8_t* pre_master_secret, int in_size, int *out_size){
    RSA *rsa;
    uint8_t *pre_master_secret_encrypted;
    
    rsa = NULL;
    pre_master_secret_encrypted = NULL;
    
        switch(algorithm){
            case RSA_:
                rsa = EVP_PKEY_get1_RSA(pKey);
                pre_master_secret_encrypted = (uint8_t*)calloc(RSA_size(rsa), sizeof(uint8_t));
                *out_size = RSA_public_encrypt(48, pre_master_secret, pre_master_secret_encrypted, rsa, RSA_PKCS1_PADDING);//TODO: rivedere sto padding
                break;
            case DIFFIE_HELLMAN:
                printf("CIAO");
                break;
            case KFORTEZZA:
                printf("CIAO");
                break;
            default:
                perror("EncryptPreMaster error: unknown keyexchange algorithm.");
                exit(1);
                break;
        }
        
        return pre_master_secret_encrypted;
}
uint8_t* decryptPreMaster(KeyExchangeAlgorithm alg, uint8_t *enc_pre_master_secret, int in_size, int *out_size){
    uint8_t *pre_master_secret;
    FILE *certificate;
    RSA *rsa_private_key;
    
    rsa_private_key = NULL;
    certificate = NULL;
    pre_master_secret = (uint8_t*)calloc(48, sizeof(uint8_t));
    

    switch(alg){
        case RSA_:
            certificate = fopen("certificates/RSA_server.key","rb");
            
            if (certificate == NULL){
                printf("decryptPreMaster error: memory leak - null pointer .");
                exit(1);
            }
            
            rsa_private_key = PEM_read_RSAPrivateKey(certificate, &rsa_private_key, NULL, NULL);
            *out_size = RSA_private_decrypt(in_size, enc_pre_master_secret, pre_master_secret, rsa_private_key, RSA_PKCS1_PADDING);
            break;
            
        case DIFFIE_HELLMAN:
            break;
        case KFORTEZZA:
            break;
        default:
            perror("decryptPreMaster error: unknown key exchange algorithm.");
            exit(1);
            break;
    }
    return pre_master_secret;
}

//Symmetric TODO: TO CHECK
uint8_t* DecEncryptPacket(uint8_t *in_packet, int in_packet_len, int *out_packet_len, CipherSuite2 *cipher_suite, uint8_t* key_block, Talker key_talker, int state){
    uint8_t *out_packet;
    EVP_CIPHER_CTX *ctx;
    uint8_t *key, *iv;
    uint8_t shift1, shift2;
    
    
    shift1 = 0;
    shift2 = 0;
    key = NULL;
    iv = NULL;
    out_packet = NULL;
    
    ctx = EVP_CIPHER_CTX_new();
    if (cipher_suite->exportable) {
    	//TODO RIVEDERE
        
    }
    else{
        if (key_talker == server) {
            shift1 = cipher_suite->key_material;
            shift2 = cipher_suite->iv_size;
        }
        key = key_block + (2*cipher_suite->signature_size + shift1);
        iv = key + (2*cipher_suite->key_material + shift2);
    }

    
    switch (cipher_suite->cipher_algorithm) {
        case CNULL:
            //TODO da gestire
            break;
            
        case RC4:
            switch (cipher_suite->key_material) {
                case 5:
                    EVP_CipherInit_ex(ctx, EVP_rc4(), NULL, key, iv, state);
                    EVP_CIPHER_CTX_set_key_length(ctx, 40);
                    break;
                case 16:
                    //beta
                    EVP_CipherInit_ex(ctx, EVP_rc4(), NULL, key, iv, state);
                    break;
                    
                default:
                    perror("DecEncryptPacket error: RC4 size not corrected.\n");
                    exit(1);
                    break;
            }
            break;
            
        case RC2:
            EVP_CipherInit_ex(ctx, EVP_rc2_40_cbc(), NULL, key, NULL, state);
            break;
        
        case IDEA:
            EVP_CipherInit_ex(ctx, EVP_idea_cbc(), NULL, key, iv, state);
            break;
        
        case DES40:
            EVP_CipherInit_ex(ctx, EVP_des_cbc(), NULL, key, iv, state);
            EVP_CIPHER_CTX_set_key_length(ctx, 40);
            break;
        
        case DES:
            EVP_CipherInit_ex(ctx, EVP_des_cbc(), NULL, key, iv, state);
            break;
        
        case DES3: //TODO da sistemare
            EVP_CipherInit_ex(ctx, EVP_des_ede3_cbc(), NULL, key, iv, state);

            break;
        
        default:
            perror("DecEncryptPacket error: unknown cipher algorithm.");
            exit(1);
            break;
    }
    int tmp_len;
    
    out_packet = calloc(1024, sizeof(uint8_t)); //TODO: ALLOCO IL MAX
    
    EVP_CipherUpdate(ctx, out_packet, out_packet_len, in_packet, in_packet_len);
    EVP_CipherFinal_ex(ctx, out_packet, &tmp_len); //TODO non si capisce a che serve sta tmp_len
    *out_packet_len = tmp_len + *out_packet_len;
    printf("OUT:%d\n", *out_packet_len);
    printf("TEMP:%d\n", tmp_len);
    EVP_CIPHER_CTX_free(ctx);
    
    return out_packet;
    
}


uint8_t* MAC(CipherSuite2 cipher, Handshake *hand, uint8_t* macWriteSecret){
    
    MD5_CTX md5,md52;
    SHA_CTX sha, sha2;
    uint64_t seq_num = 1;
    uint32_t len = hand->length - 4;
    
    
    if(cipher.signature_algorithm==SHA1_){
        
        uint8_t *sha_fin;
        sha_fin = calloc(20, sizeof(uint8_t));
        
        SHA1_Init(&sha);
        SHA1_Init(&sha2);
        SHA1_Update(&sha, macWriteSecret, 16*sizeof(uint8_t));
        SHA1_Update(&sha,pad_1, sizeof(pad_1));
        SHA1_Update(&sha, &seq_num, sizeof(uint64_t));
        SHA1_Update(&sha, &hand->msg_type , sizeof(uint8_t));
        SHA1_Update(&sha, &len, sizeof(uint32_t));
        SHA1_Update(&sha, hand->content, (hand->length - 4)*sizeof(uint8_t));
        SHA1_Final(sha_fin,&sha);
        
        SHA1_Update(&sha2,macWriteSecret, 16*sizeof(uint8_t));
        SHA1_Update(&sha2,pad_2, sizeof(pad_2));
        SHA1_Update(&sha2, sha_fin,20*sizeof(uint8_t));
        
        SHA1_Final(sha_fin,&sha2);
            
        return sha_fin;
        
    }
    else if(cipher.signature_algorithm==MD5_1){
        
        MD5_Init(&md5);
        MD5_Init(&md5);
        uint8_t *md5_fin;
        md5_fin = calloc(16, sizeof(uint8_t));
        
        
        MD5_Init(&md5);
        MD5_Init(&md52);
        MD5_Update(&md5, macWriteSecret, 16*sizeof(uint8_t));
        MD5_Update(&md5,pad_1, sizeof(pad_1));
        MD5_Update(&md5, &seq_num, sizeof(uint64_t));
        MD5_Update(&md5, &hand->msg_type ,sizeof(uint8_t));
        MD5_Update(&md5, &len, sizeof(uint32_t));
        MD5_Update(&md5,hand->content, (hand->length - 4)*sizeof(uint8_t));
        MD5_Final(md5_fin,&md5);
        
        MD5_Update(&md52,macWriteSecret, 16*sizeof(uint8_t));
        MD5_Update(&md52,pad_2, sizeof(pad_2));
        MD5_Update(&md52, md5_fin, 16*sizeof(uint8_t));
        
        MD5_Final(md5_fin, &md52);
            
        return md5_fin;
    }
    else{
        perror("MAC Error: signature algorithm not valid.");
        exit(1);
    }
}















