#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")
//file saving function
void file_saver(SOCKET client, char *buffer, char *filename, int content_start, int received, int content_length) {
    //open file in binary write mode
	FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Failed to open file for writing: %s\n", filename);
        return;
    }

    // Write the initial chunk of file data
    int initial_data_size = received - content_start;
    if (initial_data_size > 0) {
        fwrite(buffer + content_start, 1, initial_data_size, file);
        content_length -= initial_data_size;
    }

    // Continue receiving the rest of the file
    char temp_buffer[4096];
    while (content_length > 0) {
        int bytes_to_read = (content_length < sizeof(temp_buffer)) ? content_length : sizeof(temp_buffer);
        int bytes = recv(client, temp_buffer, bytes_to_read, 0);
        if (bytes <= 0) {
            printf("Connection closed or error occurred during file transfer.\n");
            break;
        }
        fwrite(temp_buffer, 1, bytes, file);
        content_length -= bytes;
    }

    fclose(file);
    printf("File '%s' saved successfully.\n", filename);
}
//extract the boundary,the file content starts after the boundary
char* find_boundary(char* buffer, int buffer_size) {
    // Look for Content-Type header with boundary
    char* content_type = strstr(buffer, "Content-Type:");
    if (!content_type) return NULL;
    
    char* boundary_start = strstr(content_type, "boundary=");
    if (!boundary_start) return NULL;
    
    boundary_start += 9; // Skip "boundary="
    
    // Extract boundary value
    static char boundary[256];
    int i = 0;
    while (boundary_start[i] && boundary_start[i] != '\r' && boundary_start[i] != '\n' && i < 255) {
        boundary[i] = boundary_start[i];
        i++;
    }
    boundary[i] = '\0';
    
    return boundary;
}
//receiver gets called two times-1)receiving fil 2)esending form
void receiver(SOCKET client, char* buffer, int received){
	//if the file is being sent by the browser
    if (strncmp(buffer, "POST", 4) == 0) {
        printf("Processing POST request (file upload)...\n");
        
        // Call receive with the already received data
        char full_buffer[12000];
        memcpy(full_buffer, buffer, received);
        full_buffer[received] = '\0';

        printf("Received %d bytes\n", received);

        long long content_length = 0;
        char filename[256] = {0};
        int header_end = -1;
        
        // Find HTTP header end
        for (int i = 0; i < received - 3; i++) {
            if (full_buffer[i] == '\r' && full_buffer[i+1] == '\n' &&
                full_buffer[i+2] == '\r' && full_buffer[i+3] == '\n') {
                header_end = i + 4;
                break;
            }
        }

        if (header_end == -1) {
            printf("Invalid HTTP header (no \\r\\n\\r\\n found).\n");
            return;
        }
        // Extract Content-Length
        char* content_len_pos = strstr(full_buffer, "Content-Length:");
        if (content_len_pos && content_len_pos < full_buffer + header_end) {
            content_len_pos += 15; // Skip "Content-Length:"
            while (*content_len_pos == ' ') content_len_pos++;
            content_length = atoll(content_len_pos);
            printf("Content-Length: %lld\n", content_length);
        }
        // Get boundary for multipart data
        char* boundary = find_boundary(full_buffer, header_end);
        if (!boundary) {
            printf("No boundary found in multipart data.\n");
            return;
        }
        printf("Boundary: %s\n", boundary);

        // Find the file part in multipart data
        char boundary_marker[300];
        snprintf(boundary_marker, sizeof(boundary_marker), "--%s", boundary);
        
        char* file_part_start = strstr( full_buffer+header_end, boundary_marker);
        if (!file_part_start) {
            printf("File part not found.\n");
            return;
        }
        
        // Move to the next line after boundary
        file_part_start = strstr(file_part_start, "\r\n");
        if (!file_part_start) return;
        file_part_start += 2;

        // Extract filename from Content-Disposition header
        char* content_disp = strstr(file_part_start, "Content-Disposition:");
        if (content_disp) {
            char* filename_pos = strstr(content_disp, "filename=\"");
            if (filename_pos) {
                filename_pos += 10; // Skip 'filename="'
                int i = 0;
                while (filename_pos[i] && filename_pos[i] != '"' && i < sizeof(filename) - 1) {
                    filename[i] = filename_pos[i];
                    i++;
                }
                filename[i] = '\0';
                printf("Extracted filename: '%s'\n", filename);
            }
        }

        if (filename[0] == '\0') {
            strcpy(filename, "uploaded_file.bin");
            printf("Filename not found. Using fallback name.\n");
        }

        // Find the actual file content start (after the empty line in multipart section)
        char* content_start_marker = strstr(file_part_start, "\r\n\r\n");
        if (!content_start_marker) {
            printf("Could not find file content start.\n");
            return;
        }
        content_start_marker += 4; // Skip \r\n\r\n

        int content_start = content_start_marker - full_buffer;
        printf("File content starts at position: %d\n", content_start);

        // Calculate actual file size (exclude the final boundary)
        char end_boundary[300];
        snprintf(end_boundary, sizeof(end_boundary), "\r\n--%s--", boundary);
        
        // Find end boundary to calculate actual file size
        char* end_boundary_pos = strstr(full_buffer + content_start, end_boundary);
        int actual_file_size;
        
        if (end_boundary_pos) {
            actual_file_size = end_boundary_pos - (full_buffer + content_start);
            printf("Actual file size: %d bytes\n", actual_file_size);
        } else {
            // If end boundary not found in current buffer, use content_length minus headers
            actual_file_size = content_length - (content_start - header_end);
            printf("End boundary not found in buffer, estimated file size: %d bytes\n", actual_file_size);
        }

        // Save the file
        file_saver(client, full_buffer, filename, content_start, received, actual_file_size);
        
        // Send response
        char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 21\r\n\r\nFile uploaded successfully!";
        send(client, response, strlen(response), 0);
    } 
    //if browser doesnt send data ,server sends the form
	else {
        printf("Serving upload form...\n");
        char file_in[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 215\r\n"
            "\r\n"
            "<!DOCTYPE html>"
            "<html><body>"
            "<h2>Send A file</h2>"
            "<form method=\"POST\" enctype=\"multipart/form-data\">"
            "<input type=\"file\" name=\"file\"><br><br>"
            "<input type=\"submit\" value=\"send\">"
            "</form>"
            "</body></html>";
        send(client, file_in, strlen(file_in), 0);
    }
}

void extract_file_name(char *path,char *filename){
	//iterate to find the last slash to obtain filename;
	char *name_start=path;
	char *end=path;
	while(*end!='\0'){
		if(*end=='\\' || *end=='/')
		name_start=end;
		end++;
	}
	name_start++;
	strcpy(filename,name_start);
}
//when sending a sile from server to browser,we use http protocol which requires type of file
const char* get_mime_type(const char* filename) {
    // Find content type to transfer
    const char* ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";
    
    // Convert to lowercase for comparison
    if (_stricmp(ext, ".txt") == 0) return "text/plain";  // Windows uses _stricmp
    if (_stricmp(ext, ".html") == 0) return "text/html";
    if (_stricmp(ext, ".css") == 0) return "text/css";
    if (_stricmp(ext, ".js") == 0) return "application/javascript";
    if (_stricmp(ext, ".json") == 0) return "application/json";
    if (_stricmp(ext, ".jpg") == 0) return "image/jpeg";
    if (_stricmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (_stricmp(ext, ".png") == 0) return "image/png";
    if (_stricmp(ext, ".gif") == 0) return "image/gif";
    if (_stricmp(ext, ".pdf") == 0) return "application/pdf";
    if (_stricmp(ext, ".zip") == 0) return "application/zip";
    if (_stricmp(ext, ".mp4") == 0) return "video/mp4";
    if (_stricmp(ext, ".mp3") == 0) return "audio/mpeg";
    
    return "application/octet-stream";
}
//the typical send function wont guarntee that everything is sent out so we use a custom sending function
int send_all(SOCKET client, char *buffer, size_t length){
	size_t total=0;
	while(total<length){
		int sent=send(client, buffer+total, (int)(length-total), 0);
		if(sent<=0) break;
		total+=sent;
	}
	return total;
}
//this function retrives filename,type and send it the client
void file_sender(SOCKET client, char *path, const char *mime_type, char *filename){
	FILE *file=fopen(path,"rb");
	if(!file){
		printf("\n the file doesnt exist");
		return;
	}
	fseek(file,0,SEEK_END);
	long filesize=ftell(file);
	fseek(file,0,SEEK_SET);
	char headers[1024];
    int header_len = snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Content-Disposition: attachment; filename=\"%s\"\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n",
        mime_type, filesize, filename);
        if (send_all(client, headers, header_len) < 0) {
        printf("Error: Failed to send HTTP headers\n");
    }
    char sender_buffer[8192];
    size_t bytes=0;
    while((bytes=fread(sender_buffer,1,sizeof(sender_buffer),file))>0){
    	send_all(client, sender_buffer, bytes);
	}
	Sleep(300);//giving enough time for the data in the buffer to be sent to the browser
	fclose(file);
}

void sender(SOCKET client, char *path){
char filename[250];//storing the path we got from choice function and passing it to the helper functions
char path_str[300];
strcpy(path_str,path);
//helper functions
extract_file_name(path_str,filename);//extract filename from path and store it in filename
const char *mime_type=get_mime_type(filename);
file_sender(client, path_str, mime_type, filename);
}
//this function for selecting choices
void choice_func(SOCKET client) {
    char ini_buffer[4096];
    int ini_bytes = recv(client, ini_buffer, sizeof(ini_buffer) - 1,0);
    if (ini_bytes <= 0) {
        return;
    }
    ini_buffer[ini_bytes] = '\0';
    //printf("%s",ini_buffer);for debugging the browser response its not the part of the program
    // Check if this is the initial request (GET / or similar)
      if (strstr(ini_buffer, "GET / HTTP") && !strstr(ini_buffer, "option=")){
        printf("\n=== Sending HTML form to client ===\n");
        // First response: Show the buttons(send/receive)
        const char* html_body =
            "<!DOCTYPE html>"
            "<html><body>"
            "<h2>Choose an action:</h2>"
            "<form action=\"/action\" method=\"get\">"
            "<button type=\"submit\" name=\"option\" value=\"send\">Send File</button>"
            "<button type=\"submit\" name=\"option\" value=\"receive\">Receive File</button>"
            "</form>"
            "</body></html>";
        
        int content_length = strlen(html_body);
        
        char response[2048];
        int response_len = snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s", content_length, html_body);
        
        send(client, response, response_len, 0);
  }
    else if (strstr(ini_buffer, "option=send")) {//if the send button is clicked the browser response contains send,we simply check it
                receiver(client, ini_buffer, ini_bytes);//if browser wants to send data then server(ex pc) needs to receive it ,thst is why receiver is called
            }
    else if (strstr(ini_buffer, "option=receive")) {//if browser wants to recieve,pc/lap(server) needs to send the file 
                char path[150];
		      	printf("\nEnter the filename with path: ");//get filepath and filename store in the the path
                fgets(path, sizeof(path), stdin);//filename may contain so we need to use fgets
                path[strcspn(path, "\n")] = '\0';
			    printf("\n %s",path); //prints the path just for debugging not a part of code
			    sender(client,path);//calls the sender function
            }
    else{
    	return;
	}
}
int main() {
    WSADATA wsa;
    SOCKET testSock;//server socket
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed. Error: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Winsock initialized!\n");

    testSock = socket(AF_INET, SOCK_STREAM, 0); //create a socket
    if (testSock == INVALID_SOCKET) {
        printf("Socket creation failed. Error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Socket created successfully.\n");

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;//it means ipv4 is used
    serverAddr.sin_port = htons(9090);//port server listens to
    serverAddr.sin_addr.s_addr = INADDR_ANY;//server can listen on any interface-localhost,lan
     //bind the server with the socket alon with these parameters
    if (bind(testSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed. Error: %d\n", WSAGetLastError());
        closesocket(testSock);
        WSACleanup();
        return 1;
    }
    printf("Socket bound to port 9090 successfully.\n");
    //make the server to listen,no represent max clients which can be waiting in queue
    if (listen(testSock, 1) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(testSock);
        WSACleanup();
        return 1;
    }
    printf("Server is listening on port 9090...\n");

    while (1) {
        struct sockaddr_in clientAddr;//create client socket 
        int client_size = sizeof(clientAddr);
        //accept the client 
        SOCKET client = accept(testSock, (struct sockaddr*)&clientAddr, &client_size);
        if (client == INVALID_SOCKET) {
            printf("Accept error: %d\n", WSAGetLastError());
            continue;
        }
        choice_func(client); //call choice
       closesocket(client);//close connection
    }
    closesocket(testSock);
    WSACleanup();
    return 0;
}

