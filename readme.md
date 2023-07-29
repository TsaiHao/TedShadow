# TedShadow 

TedShadow is a software tool designed to automate the process of downloading TED Talks videos and playing them sentence by sentence. It is primarily aimed at English learners, offering them a unique and interactive way to improve their language skills by following along with TED Talks speakers. The software is built using C++, libSDL2, and FFmpeg.

## Supported Platforms

Currently, TedShadow is compatible with Linux distributions(tested on Fedora 38) and macOS. The software currently offers an interactive command-line interface, and we are actively working on providing a GUI.

## How to Use

Follow the steps below to compile and use it:

1. Clone the Git repository:
```bash 
   git clone git@github.com:TsaiHao/TedShadow.git
```
2. Navigate to the repository:
```bash
   cd TedShadow
```
3. Create and navigate to the build directory:
```bash
   mkdir build && cd build
```
4. Compile the source code:
```bash
   cmake .. && make
```
5. To run unit tests:
```bash
   cmake .. -DTS_ENABLE_TESTS=ON
```


## Contributing

Contributions to TedShadow are very welcome! If you have a feature request, bug report, or have created a patch, please feel free to open an issue or a pull request.

## Acknowledgements

A big thank you to the TED Talks team for their inspiring content, and to the creators of libSDL2 and FFmpeg for their amazing tools.
