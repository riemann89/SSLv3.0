#include "SSL_functions.h"

/*****************************************FUNCTIONS***********************************************/

//CHANNEL FUNCTIONS

/*
 It allows the communication to the indicated talker: (0 - client, 1 - server, as defined in Talker enum)
 */
void OpenCommunication(Talker talker){
    //VARIABLE DECLARATION//
    FILE* token;
    //CHECKING INPUT//
    if (talker!=client && talker!=server) {
        perror("Error in OpenCommunication -  Error in talker input (nor client, nor server input)");    //problema a incolonnare non si può spezzare la stringa
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

/*
 It checks who between server/client can communicate. It returns the authorized user that can communicate over the channel.
 */
Talker CheckCommunication(){
    
    //VARIABLES DECLARATION
    FILE* token;
    Talker authorized_talker;																													//returning variable
    token=fopen("token.txt", "r");    																										//on token.txt is saved the current authorized talker 
    if(token == NULL) {
        perror("Failed to open token.txt - CheckCommunication(client) operation");							//is the file empty?? it shouldn't
        exit(1);
    }
	//SEEK WICH ONE IS AUTHORIZETD TO TALK//
    fscanf(token,"%u",&(authorized_talker));																						//read value from talker and insert it into the returning variable
    fclose(token);																																		//Close file
    if (authorized_talker!=client && authorized_talker!=server) {
        perror("Error in token.txt - nor client,nor server authorized");													//is the read value an acceptable value?? in case not return an error.
        exit(1);
    }
    return authorized_talker;
}

/*
 This function load a certificate from a file and return an array of bites where are contained certificate information in DER format
 */
 
Certificate* loadCertificate(char * cert_name){
    
    Certificate *certificate;
    certificate = calloc(1,sizeof(certificate));
    X509* certificate_x509 = NULL;
    uint8_t *buf;
    
    int len;
    
    buf = NULL;
    FILE* f = fopen(cert_name, "r");
    
    if (f != NULL){
        certificate_x509 = PEM_read_X509(f, NULL, 0, NULL);
        len = i2d_X509(certificate_x509, &buf);
        if (len < 0){
            perror("in loadCertificate: Certificate Not Valid\n");
            exit(1);
        }
    }
    certificate->X509_der = buf;
    certificate->len = len;
    return certificate;
}
/*
 It writes each fields of the record_layer struct, pointed by the input, over SSLchannel.txt file.
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
        perror("Failed to open SSLchannel.txt - sendPacket operation");										//error
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

/***************************************FREE FUNCTIONS**********************************************/

void FreeRecordLayer(RecordLayer *recordLayer){
    free(recordLayer->message);
    free(recordLayer);
}

void FreeHelloRequest(HelloRequest *hello_request){
    free(hello_request);
}

void FreeHandshake(Handshake *handshake){
    free(handshake->content);
    free(handshake);
}

void FreeClientServerHello(ClientServerHello *client_server_hello){
    free(client_server_hello->ciphersuite);
    free(client_server_hello);
}
/*
void FreeCertificate(Certificate *certificate){
    free(certificate->X509_der);
    free(certificate);
}
*/
void FreeCertificateRequest(CertificateRequest *certificate_request){
    free(certificate_request->certificate_authorities);
    free(certificate_request);
}

void FreeServerDone(ServerDone *server_done){
    free(server_done);
}

void FreeCertificateVerify(CertificateVerify *certificate_verify){
    free(certificate_verify->signature);
    free(certificate_verify);
}

void FreeClientKeyExchange(ClientKeyExchange *client_key_exchange){
    free(client_key_exchange->key_exchange);
    free(client_key_exchange);
}

void FreeCertificateFinished(Finished *finished){
    free(finished);
}//TODO ricontrollare questa free

/********************FUNCTION TO CONSTRUCT HANDSHAKE PROTOCOL MESSAGE TYPES*************************/
/* Message types to Handshake */
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

Handshake *ClientServerHelloToHandshake(ClientServerHello *client_server_hello){
    //VARIABLE DECLARATION//
    CipherSuite *cipher;
    Handshake *handshake; 																		 	//returning variable
    uint8_t timeB[4]; 																							//current time bytes representation
    uint8_t session[4];			 																				//session bytes representation
    uint8_t cipher_codes[client_server_hello->length-38];    							// 38= Random(32)+session(4) + version(1) + length(1)  //array of all ciphers supported
    uint8_t *Bytes;																								//Used to serialize various fields of ClientServerHello and then pass to Handshake->content field
    //MEMORY ALLOCATION//
    Bytes =(uint8_t*)calloc(client_server_hello->length
	,sizeof(uint8_t));																								 //bytes data vector, as said Bytes is an array which represents client_server_hello
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
    cipher=client_server_hello->ciphersuite;   													 //temporary vector containing all cipher codes - it is requested to perform following memcopy
    for (int i=0;i<(client_server_hello->length-38);i++){  
        cipher_codes[i]=(cipher+i)->code;
    }
    int_To_Bytes(client_server_hello->random.gmt_unix_time, timeB);   	    //unix_time 
    int_To_Bytes(client_server_hello->sessionId, session);  								// session values to bytes transformation
    Bytes[0]=client_server_hello->version;   														//serializing client/server_hello field into bytes data vector
    memcpy(Bytes+1 ,session, 4);
    memcpy(Bytes+5 ,timeB , 4);
    memcpy(Bytes+9,client_server_hello->random.random_bytes,28);
    memcpy(Bytes+37, cipher_codes,client_server_hello->length-38);       		//38= version(1)+length(1)+session(4)+random(32)
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = client_server_hello->type;   												//handshake fields initialization
    handshake->length = client_server_hello->length + 3;
    handshake->content = Bytes;
    return handshake;
}
/*
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
*/
Handshake *ServerKeyExchangeToHandshake(ServerKeyExchange *server_key_exchange){
    Handshake *handshake;
    uint8_t *Bytes;
    int key_exchange_algorithm;
    int parameters_size;
    int signature_algorithm;
    int signature_size;
    int key_exchange_size;
    
    key_exchange_algorithm = server_key_exchange->parameters->algorithm_type;
    parameters_size = server_key_exchange->parameters->size;
    signature_algorithm = server_key_exchange->signature->algorithm_type;
    signature_size = server_key_exchange->signature->size;
    key_exchange_size = parameters_size + signature_size;
    
    Bytes = (uint8_t*)calloc(parameters_size + signature_size, sizeof(uint8_t));//TODO
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
    memcpy(Bytes,server_key_exchange->parameters->parameters, parameters_size);
    memcpy(Bytes+parameters_size, server_key_exchange->signature->signature, signature_size);
    
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = SERVER_KEY_EXCHANGE;
    handshake->length = 4 + key_exchange_size;
    handshake->content = Bytes;
    
    return handshake;
}

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
};

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
    handshake->length = 5;
    handshake->content = Bytes;
    return handshake;
}

Handshake *CertificateVerifyToHandshake(CertificateVerify *certificate_verify){
    Handshake *handshake;
    uint8_t *Bytes;
    int bytes_size;
    
    switch (certificate_verify->algorithm_type) {
        case SHA1_:
            bytes_size = 20;
            break;
        case MD5:
            bytes_size = 16;
        default:
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
};

Handshake *ClientKeyExchangeToHandshake(ClientKeyExchange *client_key_exchange){
    Handshake *handshake;
    uint8_t *Bytes;
    
    Bytes = (uint8_t*)calloc(client_key_exchange->len_key_exchange, sizeof(uint8_t));//TODO
    if (Bytes == NULL) {
        perror("ClientKeyExchangeToHandshake: Failed to create Bytes pointer");
        exit(1);
    }
    
    handshake=(Handshake*)calloc(1,sizeof(handshake));
    if (handshake == NULL) {
        perror("ClientKeyExchangeToHandshake:: Failed to create handshake pointer");
        exit(1);
    }
    
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    memcpy(Bytes, client_key_exchange->key_exchange, client_key_exchange->len_key_exchange);
    
    //HANDSHAKE CONSTRUCTION//
    handshake->msg_type = CLIENT_KEY_EXCHANGE;
    handshake->length = 4 + client_key_exchange->len_key_exchange;
    handshake->content = Bytes;
    
    return handshake;
};

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
}//TODO TEST

/* Handshake to message types */

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
    }//TOCHECK

ClientServerHello *HandshakeToClientServerHello(Handshake *handshake){
    ClientServerHello *client_server_hello;
    
    if (handshake->msg_type != CLIENT_HELLO || handshake->msg_type != SERVER_HELLO){
        perror("HandshakeToClientServerHello: handshake does not contain an client_hello/server_hello message.");
        exit(1);
    }
    
    client_server_hello = (ClientServerHello*)calloc(1, sizeof(ClientServerHello));
    
    client_server_hello->length = handshake->length-4;
    client_server_hello->version = handshake->content[0];
    //client_server_hello->random = 1; //TODO
    //client_server_hello->sessionId = //TODO
    return client_server_hello;
};//RIVEDERE da completare

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
};//TOCHECK

/*
ServerKeyExchange *HandshakeToServerKeyExchange(Handshake *handshake){
    ServerKeyExchange *server_key_exchange;
    KeyExchangeParameters *parameters;
    KeyExchangeSignatures *signature;
    int certificate_len;
    
    if (handshake->msg_type != SERVER_KEY_EXCHANGE){
        perror("HandshakeToServerKeyExchange: handshake does not contain a server key exchange message.");
        exit(1);
    }
    
    certificate_len = handshake->length - 4;
    
    server_key_exchange = (ServerKeyExchange *)calloc(1, sizeof(ServerKeyExchange));
    parameters = (KeyExchangeParameters *)calloc(1, sizeof(KeyExchangeParameters));
    signature = (KeyExchangeSignatures *)calloc(1, sizeof(KeyExchangeSignatures));
    
    parameters->algorithm_type =
    
    memcpy(buffer, handshake->content, certificate_len);
    
    certificate->len = certificate_len;
    certificate->X509_der = buffer;
    
    return certificate;

};//TODO : Probabilmente va rivista la struttura, perchè dall'handshake, non riusciamo a ricavare il tipo di algoritmo.E quindi è impossibile completare tutti i campi.

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
    
    certificate->len = certificate_len;
    certificate->X509_der = buffer;
    
    return certificate;
};//TODO Rivedere: non riesco a rappresentare le liste, rivedere anche la struttura a questo punto.

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
            certificate_verify->algorithm_type = MD5;
            break;
        default:
            perror("ERROR HandshakeToCertificateVerify: signature size not valid.");
            exit(1);
            
    }
    
    memcpy(signature, handshake->content, signature_len);
    
    return certificate_verify;
    
};//TOCHECK

ClientKeyExchange *HandshakeToClientKeyExchange(Handshake *handshake);//TODO

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


};//TOCHECK

/* Handshake to RecordLayer */
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
    recordlayer = (RecordLayer*)calloc(handshake->length +
 5,sizeof(RecordLayer));  		 																										//record layer allocation memory i need 5 extra-bytes  
    if (recordlayer == NULL) {
        perror("Failed to create recordlayer pointer - HandshakeToRecordLayer operation");
        exit(1);
    }
    //CONTENT BYTES DATA VECTOR CONSTRUCTION//
    int_To_Bytes(handshake->length ,length24); 			  				  												//int of 4 bytes to int of 3 bytes and reversed
    len=handshake->length;							
    Bytes[0]=handshake->msg_type;																									//serializing handshake and store it into Bytes
    memcpy(Bytes+1 ,length24+1,3);                										 											// length24 + 1 cause i need only the last 3 bytes
    memcpy(Bytes+ 4 ,handshake->content,len-4); 																			// +4 since 4=type(1)+length(3)		
	//RECORDLAYER CONSTRUCTION//
    recordlayer->type=HANDSHAKE;																									//recordlayer fields initialization
    recordlayer->version=std_version;
    recordlayer->length=handshake->length+5;
    recordlayer->message=Bytes;
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
    
    FreeRecordLayer(record);
    return result;
    
}

/* funzione per leggere il file*/

RecordLayer  *readchannel2(){
	uint8_t *buffer;
    uint8_t record_header[5];//rivedere
    FILE* SSLchannel;
    uint32_t packet_size;
    RecordLayer *returning_record;
    ContentType type;
    ProtocolVersion version;
    
    SSLchannel=fopen("SSLchannelbyte.txt", "r");
	if(SSLchannel==NULL)
	{
		printf("Error unable to read the SSLchannel");
        exit(1);
	}
    
    fread(record_header, sizeof(uint8_t), 5*sizeof(uint8_t), SSLchannel);
    packet_size = Bytes_To_Int(2, record_header + 3);
    buffer = (uint8_t*)malloc((packet_size - 5)*sizeof(uint8_t)); //alloc enough memory to handle SSLchannel file
    fseek(SSLchannel, SEEK_SET, 5);
	fread(buffer,sizeof(uint8_t),(packet_size-5)*sizeof(uint8_t), SSLchannel);// load file into buffer
    returning_record = calloc(6,sizeof(uint8_t));
    
	type = record_header[0];
	returning_record->type = type;	// assign type

	version.minor = record_header[1]; //loading version
	version.major = record_header[2];
	returning_record->version= version;// assign version
	returning_record->length = (uint16_t)packet_size;// assign length to record
	returning_record->message= buffer;
    return returning_record;//assign pointer to message
} //Read Channel and return the reconstructed ClientHello from wich i will get the SeverHello wich i will have to send into the channel..  TODO now just return clienthello.. does not read the  handshake in general

ClientServerHello *readchannel(){
    
    
    uint8_t *buffer;
    FILE* SSLchannel;
    SSLchannel=fopen("SSLchannelbyte.txt", "r");
    
    ClientServerHello *returning_hello;	 //returning variable
    returning_hello=(ClientServerHello*) calloc(1,sizeof(returning_hello));
    
    
    
    buffer = (uint8_t *)malloc((150)*sizeof(uint8_t));    // Enough memory for file + \0
    fread(buffer, 100, 1, SSLchannel);
    
    
    uint8_t  version=(uint8_t)*(buffer+9);
    
    
    uint8_t  length= (uint8_t)*(buffer +8) -4 ;  //tolgo i byte in più dell' handshake  (version + length)   è un po' sporca...sfrutto il fatto che la lunghezza sta in realtà in un byte... TODO
    
    uint8_t session[4];
    
    for(int i =0;i<4;i++){
        session[i]= *(buffer + 10 + i);
    }
    reverse(session,4);   // reversing dei bytes della session
    
    uint32_t  SessionId=(uint32_t)(session[0] + session[1] *256 + session[2]*256*256 + session[3]*256*256);
    Random ran;
    ran.gmt_unix_time=time(0);  //metto il tempo nuovo in secondi.. dovrei trovare quella in millis
    
    
    for (int i =0; i<28;i++){
        
        ran.random_bytes[i]=(uint8_t)*(buffer +(18+i));
    }
    
    //uint8_t  ciphers[length - 38]; //length of  ciphers
    CipherSuite *ciphers = malloc((50)*sizeof(CipherSuite));
    
    
    
    for (int i =0; i<length-32;i++){
        printf("%d",i+18);
        ciphers[i]= get_cipher_suite(buffer[18 +28 +i]);
    }
    //uint8_t *ciphers_ptr;
    
    //ciphers_ptr=&ciphers;
    
    printf("carico ritorno");
    returning_hello->version=version;
    returning_hello->length=length;
    returning_hello->sessionId=SessionId;
    returning_hello->random=ran;
    returning_hello->ciphersuite=ciphers;
    //printf("%02x\n \n",ciphers[0].code);  //comodo come controllo
    //returning_hello->ciphersuite= (Cipher_Suite*)ciphers_ptr;
    return returning_hello;
}

//in this toy we set the priorities of the server in the file "PriorityList.txt", so that we are able to choose the best cipher supported according to that file, on this pourpose chiphersuites  are saved in decrescent order of  priority

void setPriorities(uint8_t number,uint8_t *priority){   																//numero ciphers supportati,  lista priorità da inserire in ordine decrescentenell'array priority[number]
    FILE* PriorityList; 																														//creo il file
    PriorityList = fopen("PriorityList.txt", "wb");   																			//file where will be stored the chipers supported by server in decrescent order of priority
    uint8_t *length;   																													 	//inserisco lunghezza
    length=&number;
    fwrite(length,sizeof(uint8_t),1,PriorityList);
    for(int i = 0; i<number; i++){   																									//loading chiphers
        fwrite((priority +i),sizeof(uint8_t),1,PriorityList);
    }
    fclose(PriorityList);                                                                                                                     //close file
}

//that function read from PryorityList.txt and the input struct ClientServerHello, comparing chiphers Priority and avaiable and choosing the best fitting in a naive way
uint8_t chooseChipher(ClientServerHello *client_supported_list){
    FILE* PriorityList;
    PriorityList = fopen("PriorityList.txt", "r");  	 																		//read the  priority list written by setPryorities on this file
    uint8_t *buffer ;
    buffer = (uint8_t *)malloc((32)*sizeof(uint8_t));
    fread(buffer, 32, 1, PriorityList);    																						//temporary priorities are stored here easier to manage
    uint8_t choosen;  																													//the returning variable, the choice
    for(int i=1; i<(int)buffer[0]+1; i++){          																				// check decrescently if a certain chipher is avaiable on client_supported_list
        for(int j=0;j<client_supported_list->length -38 ;j++){  
            if(buffer[i]==client_supported_list->ciphersuite[j].code){  										//check if the suite is avaiable
                choosen=buffer[i];
                return choosen;   																										//find the best possible chipher according to my list, return that one as a byte
            }
        }
    }
    printf("\nError, uncompatibles chiphers");   																		//no compatible chipher, print error
				exit(1);
}

/* about certificates*/

int writeCertificate(X509* certificate){
    /* Per leggere il der
    X509 *res= NULL;
    d2i_X509(&res, &buf, *len);
     */
    FILE* file_cert;
    
    file_cert=fopen("cert_out.crt", "w+");
    return PEM_write_X509(file_cert, certificate);
}
int readCertificate(){return 0;} //TODO ricostruisco il file del certificato da cui leggo i parametri che mi servono.