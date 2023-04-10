/*
Developping the communication code with the scanner profiler Keyence
Nss
*/



#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    // IP address of the Keyence laser profiler
    const char* ip_address = "192.168.0.10";

    // byte-structure/command for getting latest measurement values
    char LJV_GetMeasurementValues[] = { 0x14, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF0, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
                                        0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    size_t LJV_GetMeasurementValues_s = sizeof(LJV_GetMeasurementValues);

    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Address structure for the IP address
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip_address);
    servaddr.sin_port = htons(24691);

    // Connect to the controller
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Failed to connect to the Keyence laser profiler" << std::endl;
        close(sockfd);
        return 1;
    }

    // Send command
    if (send(sockfd, LJV_GetMeasurementValues, LJV_GetMeasurementValues_s, MSG_DONTROUTE) < 0) {
        std::cerr << "Failed to send command" << std::endl;
        close(sockfd);
        return 1;
    }

    // Receive response
    char recvline[1000];
    int n;
    usleep(5 * 10000);
    n = read(sockfd, recvline, 1000);
    if (n < 0) {
        std::cerr << "Failed to receive response" << std::endl;
        close(sockfd);
        return 1;
    }
    recvline[n] = 0;
    std::cout << "Received " << n << " bytes in response: " << recvline << std::endl;

    // Getting measurement value for OUT1-OUT12
    int i;
    for (i = 1; i <= 10; i++) {
        int msb, lsb, byte2, byte3, outStartBytePosition, outMeasurementValue, byteOffset;
        double outMeasurementValueMM;
        byteOffset = (i - 1) * 8;
        outStartBytePosition = 232 + byteOffset;
        msb = (unsigned char)recvline[outStartBytePosition + 3];
        byte2 = (unsigned char)recvline[outStartBytePosition + 2];
        byte3 = (unsigned char)recvline[outStartBytePosition + 1];
        lsb = (unsigned char)recvline[outStartBytePosition];
        outMeasurementValue = msb << 24 | byte2 << 16 | byte3 << 8 | lsb; //shift bytes to big endian
        outMeasurementValueMM = outMeasurementValue * 0.00001; //since values are stored in 10nm units we have to multiply with factor 0.00001 to reach a mm-scale
        printf("OUT%i value = %fmm \n", i, outMeasurementValueMM); //output decimal value for OUT1-16
    }
    close(sockfd);
}

