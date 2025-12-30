# Easy yt-dlp CLI

**Easy yt-dlp CLI** is a CLI, built using C++, for [yt-dlp](https://github.com/yt-dlp/yt-dlp), a feature-rich command-line audio and video downloader, with support for most its essential options. 

## Building the CLI
1. Acquire a C++ compiler via the GNU compiler toolkit (GCC)
    - [Windows Instructions](https://code.visualstudio.com/docs/cpp/config-mingw)
    - [Linux Instructions](https://code.visualstudio.com/docs/cpp/config-linux)
2. Acquire a copy of the **Python** version of [yt-dlp](https://github.com/yt-dlp/yt-dlp)
3. Run the following: 
    1. `g++ ytdlp-cli.cpp -o ytdlp-cli`
    2. `./ytdlp-cli`