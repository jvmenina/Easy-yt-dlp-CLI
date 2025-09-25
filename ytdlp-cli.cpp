#include <iostream>
#include <cstddef>

#include <exception>
#include <stdexcept>

#include <map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string_view>

namespace Constants {
    const std::map<std::string, std::string> namingSchemes {
        {"Title and ID", "%(title)s - %(id)s.%(ext)s"},
        {"Uploader and ID", "%(uploader)s - %(id)s.%(ext)s"},
        {"Website and ID", "%(webpage_url_domain)s - %(id)s.%(ext)s"},
        {"Custom", ".%(ext)s"}
    };
}

namespace Config {
    const std::vector<std::string> configKeywordStrings {
        "ytdlp_path"
    };
    enum class configKeyword {
        YTDLP_PATH = 0
    };

    // std::string_view getKeyword(const configKeyword& index) {
    const std::string& getKeyword(const configKeyword& index) {
        return configKeywordStrings[static_cast<int>(index)];
    };
    
    const std::string defaultConfigPath {"./ytdlp-cli.ini"};
    const std::map<std::string, std::string> defaultConfig {
        {configKeywordStrings[0],"./yt-dlp"}
    };    

    void createDefaultConfigFile(const std::string& path) {
        std::ofstream configFile {path};
        std::string defaultConfigString {};
        for (std::pair<std::string, std::string> c : defaultConfig) {
            defaultConfigString.append(c.first + "=" + c.second + "\n");
        }
        configFile << defaultConfigString << std::endl;
        configFile.close();
    }
}

namespace Utils {
    void printLog(const std::string& message) {
        std::cout << ">> (LOG) " << message << std::endl;
    }

    void printError(const std::string& message) {
        std::cout << ">> (ERROR) " << message << std::endl;
    }
}

class LoadedConfig {
    const std::string ytdlp_path;
    bool locked {true};

    LoadedConfig(const std::string& ytdlp_path):
        ytdlp_path {ytdlp_path}
    {
        Utils::printLog("Config loaded.");
        this->printConfig();
    }

public:
    ~LoadedConfig() {
        if (this->locked) {
            throw std::runtime_error("Unexpected Error: loaded configuration was destroyed.");
        }
    }

    static LoadedConfig loadConfig(const std::string& path) {
        // Initialize file
        std::ifstream inputConfig {path};
        if (!inputConfig) {
            Config::createDefaultConfigFile(path);
            
            // Update input file stream
            inputConfig.close();
            inputConfig = std::ifstream{path};
        }
        
        // Get key-value pairs from file
        // std::map<std::string, std::string> configFromFile {};
        std::string ytdlp_path {Config::defaultConfig.at(Config::getKeyword(Config::configKeyword::YTDLP_PATH))};
        for (std::string temp; std::getline(inputConfig, temp);) {
            std::size_t delimIndex {temp.find('=')};
            if (delimIndex == std::string::npos) continue;

            const std::string& key {temp.substr(0,delimIndex)};
            const std::string& value {temp.substr(delimIndex+1)};
            if (!key.length() || !value.length()) continue;
            
            if (key == Config::getKeyword(Config::configKeyword::YTDLP_PATH))
                ytdlp_path = value;
            // configFromFile.insert({key, value});
        }

        // Process pairs
        // if (configFromFile.empty()) return LoadedConfig {"EMPTYCONFIG"};
        // Unlike "find", using "at" with a non-existent key throws an error
        // const std::string& ytdlp_path {configFromFile.at(Config::getKeyword(Config::configKeyword::YTDLP_PATH))};
        // const auto temp_ytdlp_path {configFromFile.find(Config::getKeyword(Config::configKeyword::YTDLP_PATH))};
        // if (temp_ytdlp_path == configFromFile.end())
        
        return LoadedConfig {ytdlp_path};
    }

    void printConfig() {
        Utils::printLog("Loaded config: " + ytdlp_path);
    }

    void closeConfig() {
        this->locked = false;
    }
};

int main() {
    // std::system("python3 yt-dlp --version");
    try
    {
        LoadedConfig lc {LoadedConfig::loadConfig(Config::defaultConfigPath)};
        lc.closeConfig();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}