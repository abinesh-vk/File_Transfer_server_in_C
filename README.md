# HTTP File Transfer Server

Ever wanted to quickly share files between your computer and any device with a web browser? This project does exactly that! It's a simple HTTP file transfer server written in C that lets you upload and download files through a web interface.

## What Does This Do?

Think of it as your own personal file sharing service that runs locally. Start the server, open your browser, and you can:

- 📤 **Upload files** from your phone/tablet/laptop directly to your PC
- 📥 **Download files** from your PC to any device on your network
- 🌐 **Works with modern web browsers** on devices connected to the same network
- ⚡ **Simple setup** - just compile and run

It's perfect for those moments when you need to quickly move files around but don't want to deal with USB cables, cloud services, or complicated file sharing apps.

## Getting Started

### Compiling the Code

**If you're comfortable with command line:**
```bash
gcc -o server server.c -lws2_32 -static-libgcc
```

**If you prefer using an IDE like Dev-C++ or Code::Blocks:**
1. Open your project settings
2. Add `-lws2_32` to compiler commands  
3. Add `-static-libgcc` to linker commands
4. Hit compile!

Don't worry about those weird flags - they just tell the compiler to include Windows networking stuff and make your program work on any Windows machine.

### Running Your Server

1. **Find your PC's IP address:**
   - Open Command Prompt
   - Type `ipconfig` and press Enter
   - Look for your WiFi adapter's IPv4 Address (something like `192.168.1.100`)

2. **Make sure all devices are on the same network:**
   - Connect your PC and phone/tablet to the same WiFi
   - Or create a hotspot on your phone and connect your PC to it
   - Or create a hotspot on your PC and connect other devices to it

3. **Run the server:**
   - Double-click your compiled `server.exe`
   - You'll see "Server is listening on port 9090..." (that's normal!)

4. **Access from any device:**
   - On your phone/tablet/other computer, open a web browser
   - Go to `http://YOUR_PC_IP:9090` (example: `http://192.168.1.100:9090`)
   - Boom! You've got your file transfer page accessible from any device on your network

## Network Setup Examples

**Scenario 1: Home WiFi**
- Connect your PC and phone to your home WiFi
- Find PC's IP: `ipconfig` → look for WiFi adapter IPv4 address
- Access from phone: `http://192.168.1.100:9090` (use your actual IP)

**Scenario 2: Phone Hotspot**
- Turn on hotspot on your phone
- Connect your PC to the phone's hotspot
- Find PC's IP: `ipconfig` → look for WiFi adapter IPv4 address
- Access from another device connected to the same hotspot

**Scenario 3: PC Hotspot**
- Enable Mobile Hotspot on your PC (Windows 10/11)
- Connect your phone/tablet to the PC's hotspot
- Use the PC's hotspot IP address (usually `192.168.137.1:9090`)

## How to Use It

Once you're on the web page, you'll see two simple buttons:

**Send File** - Click this to upload a file from your browser to your computer. Great for getting photos off your phone!

**Receive File** - Click this when you want to download something from your computer. The program will ask you to type the file path in the console window.

## The Cool Technical Stuff

This isn't just a simple file copy program - it's actually implementing real web server technology:

- **HTTP/1.1 Protocol** - The same standard that powers websites
- **Multipart Form Handling** - Properly processes file uploads like a real web server  
- **MIME Type Detection** - Automatically figures out if you're sending a photo, document, or video
- **Socket Programming** - Low-level network communication (the hard way!)

## What Files Can You Transfer?

Pretty much anything! The server is smart enough to handle:
- Photos and images (JPG, PNG, GIF)
- Documents (PDF, TXT, HTML)  
- Videos and music (MP4, MP3)
- Archives (ZIP files)
- Really, any file type you throw at it

## A Few Things to Keep in Mind

This server is designed for local use and learning. It's not meant to be exposed to the internet or used in production environments. Think of it more like a cool tech demo that actually does useful stuff!

If you're getting files from your computer, you'll need to type the full path (like `C:\Users\YourName\Documents\file.txt`) when the program asks.

## What You Need

- A Windows computer (this uses Windows-specific networking)
- A C compiler (GCC, MinGW, Dev-C++, or Visual Studio)
- Any web browser for the interface
- Basic comfort with compiling C programs

## How It All Works Behind the Scenes

The magic happens in a few key parts:
- `main()` sets up the server and waits for connections
- `choice_func()` figures out what the browser is asking for
- `receiver()` handles file uploads with all the HTTP complexity
- `sender()` pushes files back to your browser
- Various helper functions handle the nitty-gritty details

## Want to Contribute?

Found a bug? Want to add features? Go for it! This project is a great way to learn about network programming, HTTP protocols, and C development. Feel free to experiment with the code and share what you've learned.

The code is written to be educational, so don't be surprised if you find comments explaining the "why" behind the technical decisions.
