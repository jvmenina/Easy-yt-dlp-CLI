#include <iostream>
#include <cstddef>

#include <exception>
#include <stdexcept>

#include <filesystem>
#include <map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string_view>

#include <limits.h>

/**
 * Namespace for all non-specific contants
 */
namespace Constants {
    const std::map<std::string, std::string> outputNamingSchemes {
        {"Title and ID", "%(title)s - %(id)s.%(ext)s"},
        {"Uploader and ID", "%(uploader)s - %(id)s.%(ext)s"},
        {"Website and ID", "%(webpage_url_domain)s - %(id)s.%(ext)s"},
        {"Custom", ".%(ext)s"}
    };
}

/**
 * Namespace for general-purpose utility functions for convenience
 */
namespace GeneralUtils {
    void printLog(const std::string& message) {
        std::cout << ">> (LOG) " + message << std::endl;
    }

    void printError(const std::string& message) {
        std::cout << ">> (ERROR) " + message << std::endl;
    }

    void printPrompt(const std::string& message) {
        std::cout << ">> [PROMPT] " + message + " " << std::flush;
    }

    bool isValidFile(const std::string& path) {
        return std::filesystem::is_regular_file(path);
    }

    void resetInputStream() {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    template <typename T>
    std::string toString(const T& value)
    {
        std::ostringstream ss;
        ss << value;
        return ss.str();
    }
    
    std::string stringToLower(const std::string& in) {
        std::string out {in};
        transform(in.begin(), in.end(), out.begin(), [](unsigned char ch){return std::tolower(ch);});
        return out;
    }

    void stringToLowerInPlace(std::string& in) {
        transform(in.begin(), in.end(), in.begin(), [](unsigned char ch){return std::tolower(ch);});
    }

    void askInput(const std::string& message, std::string& str, bool toLower = false) {
        printPrompt(message);
        std::getline(std::cin, str);
        if (toLower) stringToLowerInPlace(str);
        std::cin.clear();
    }

    void askInput(const std::string& message, char& ch, bool toLower = false) {
        printPrompt(message);
        std::cin.get(ch);
        if (toLower) ch = std::tolower(static_cast<unsigned char>(ch));
        if (ch != '\n') 
            resetInputStream();
        else 
            std::cin.clear();
    }
}

/**
 * Namespace for config-specific constants and utility functions for convenience
 */
namespace ConfigUtils {
    const std::vector<std::string> configKeywordStrings {
        "ytdlp_path",
        "ytdlp_is_executable",
    };
    enum class configKeyword {
        YTDLP_PATH = 0,
        YTDLP_IS_EXECUTABLE
    };
    const std::string& getKeyword(const configKeyword& index) {
        return configKeywordStrings[static_cast<int>(index)];
    };
    
    const std::string configPath {"./ytdlp-cli.ini"};
    const std::map<std::string, std::string> defaultConfig {
        {configKeywordStrings[0],"./yt-dlp"},
        {configKeywordStrings[1],"false"}
    };
    const std::string& getDefaultConfigValue(configKeyword keyword) {
        return defaultConfig.at(getKeyword(keyword));
    }

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

/**
 * The configuration loaded when the program starts. Only one may be initialized
 * throughout the program and should persist until the end of the program.
 */
class LoadedConfig {
    static bool unique;

    std::string m_ytdlpPath;
    bool m_ytdlpIsExecutable;

    LoadedConfig(const std::string& ytdlpPath, const bool ytdlpIsExecutable):
        m_ytdlpPath {ytdlpPath},
        m_ytdlpIsExecutable {ytdlpIsExecutable}
    {
        if (!unique) throw std::runtime_error("Unexpected Error: Another LoadedConfig has been initialized.");
        GeneralUtils::printLog("Config loaded.");
        unique = false;
    }

public:
    // LoadedConfig constructor interface
    static LoadedConfig loadConfig(const std::string& path) {
        /**
         * Initialize config file
         */

        std::ifstream inputConfig {path};
        if (!inputConfig) {
            ConfigUtils::createDefaultConfigFile(path);
            inputConfig.close();
            inputConfig = std::ifstream{path};
        }
        
        /**
         * Initialize constructor arguments using defaults
         */
        
        std::string ytdlpPath {ConfigUtils::getDefaultConfigValue(ConfigUtils::configKeyword::YTDLP_PATH)};
        bool ytdlpIsExecutable {ConfigUtils::getDefaultConfigValue(ConfigUtils::configKeyword::YTDLP_IS_EXECUTABLE) == "true"};

        /**
         * Use config file to update arguments
         */

        for (std::string line; std::getline(inputConfig, line);) {
            std::size_t delimIndex {line.find('=')};
            if (delimIndex == std::string::npos) continue;

            const std::string& key {line.substr(0,delimIndex)};
            const std::string& value {line.substr(delimIndex+1)};
            if (!key.length() || !value.length()) continue;
            
            if (key == ConfigUtils::getKeyword(ConfigUtils::configKeyword::YTDLP_PATH))
                ytdlpPath = value;
            else if (key == ConfigUtils::getKeyword(ConfigUtils::configKeyword::YTDLP_IS_EXECUTABLE))
                ytdlpIsExecutable = value == "true";
        }

        return LoadedConfig {ytdlpPath, ytdlpIsExecutable};
    }

    /**
     * Core methods
     */

    ~LoadedConfig() {GeneralUtils::printLog("Loaded configuration was destroyed.");}

    const std::string& getYtdlpPath() const {return this->m_ytdlpPath;}
    bool setYtdlpPath(std::string& newPath) {
        if (!GeneralUtils::isValidFile(newPath)) return false;
        this->m_ytdlpPath = newPath;
        return true;
    }

    bool ytdlpIsExecutable() {return this->m_ytdlpIsExecutable;}
    void setYtdlpIsExecutable(bool newVal) {this->m_ytdlpIsExecutable = newVal;}

    /**
     * General utility methods
     */

    void printConfig() {
        GeneralUtils::printLog(GeneralUtils::toString(*this));
    }

    /**
     * Externally defined methods
     */

    friend std::ostream& operator<<(std::ostream& out, const LoadedConfig& config) {
        out << "LOADED CONFIGURATION:\n" 
            << "\t|--> " << ConfigUtils::configKeywordStrings[0] << " = " << config.m_ytdlpPath << "\n"
            << "\t|--> " << ConfigUtils::configKeywordStrings[1] << " = " << (config.m_ytdlpIsExecutable ? "true" : "false") << "\n"
            << "\t|-------------------------";
        return out;
    }
};
bool LoadedConfig::unique {true};

/**
 * The current program session that is initialized when the program starts. Only
 * one may be initialized throughout the pgoram and should persist until the end
 * of the program. It contains all the info input by the user on runtime.
 */
struct Session {
    // Level of interactivity of the current session
    enum class SessionMode {
        INTERACTIVE,
        FAST
    };

    // How the input link should be downloaded as
    enum class DownloadMode {
        AS_VIDEO,
        AS_AUDIO
    };
    
    static bool unique;

    LoadedConfig& loadedConfig;
    SessionMode sessionMode {SessionMode::INTERACTIVE};
    std::string inputLink {};
    std::string outputDirectory {};
    std::string outputFilename {};
    DownloadMode downloadMode {DownloadMode::AS_VIDEO};
    std::string downloadModeOptions {};

    /**
     * Core methods
     */

    Session(LoadedConfig& config):
        loadedConfig {config}
    {
        if (!unique) throw std::runtime_error("Unexpected Error: Another Session has been initialized.");
        GeneralUtils::printLog("Connected loaded config to current session.");
        this->loadedConfig.printConfig();
        GeneralUtils::printLog("Session initialized.");
        unique = false;
    }

    ~Session() {
        GeneralUtils::printLog("Loaded session was destroyed.");
    }

    /**
     * General utility methods
     */

    void printSession() {
        GeneralUtils::printLog(GeneralUtils::toString(*this));
    }

    /**
     * Externally defined methods
     */

    friend std::ostream& operator<<(std::ostream& out, const Session& session) {
        const auto printShowEmpty {[](const std::string& str){return str.length() ? str : "NOT YET CONFIGURED";}};

        out << "CURRENT SESSION DETAILS:\n" 
            << "\t|--> " << "Mode" << " = " << session.sessionMode << "\n"
            << "\t|--> " << "Input Link" << " = " << printShowEmpty(session.inputLink) << "\n"
            << "\t|--> " << "Output Directory" << " = " << printShowEmpty(session.outputDirectory) << "\n"
            << "\t|--> " << "Output Filename" << " = " << printShowEmpty(session.outputFilename) << "\n"
            << "\t|--> " << "Download Mode" << " = " << session.downloadMode << "\n"
            << "\t|--> " << "Download Options" << " = " << printShowEmpty(session.downloadModeOptions) << "\n"
            << "\t|-------------------------";
        return out;
    }

    friend std::ostream& operator<<(std::ostream& out, DownloadMode downloadMode) {
        switch (downloadMode) {
            case Session::DownloadMode::AS_VIDEO:
                out << "AS VIDEO";
                break;
            case Session::DownloadMode::AS_AUDIO:
                out << "AS VIDEO";
                break;
            default:
                out << "NOT YET CONFIGURED";
                break;
        }
        return out;
    }

    friend std::ostream& operator<<(std::ostream& out, SessionMode sessionMode) {
        switch (sessionMode) {
            case Session::SessionMode::INTERACTIVE:
                out << "INTERACTIVE MODE";
                break;
            case Session::SessionMode::FAST:
                out << "FAST MODE";
                break;
            default:
                out << "NOT YET CONFIGURED";
                break;
        }
        return out;
    }
};
bool Session::unique = true;

/**
 * General program flow functions
 */

void promptValidYtdlpPath(LoadedConfig &config) {
    GeneralUtils::printLog("Config-defined yt-dlp path is invalid.");
    
    std::string newPath {};
    while (true) {
        GeneralUtils::askInput("Input new path: ", newPath);
        if (std::cin.fail() || newPath.length() >= PATH_MAX) {
            GeneralUtils::printError("Input failed! Try again.");
            GeneralUtils::resetInputStream();
            continue;
        }
        if (config.setYtdlpPath(newPath)) break;
        GeneralUtils::printError("Invalid path! Try again.");
    }
    GeneralUtils::printLog("Updated yt-dlp path.");
    
    char isExecutableChar {};
    while (true) {
        GeneralUtils::askInput("Is executable (Y / N[default])? ", isExecutableChar, true);
        if (std::cin.fail()) {
            GeneralUtils::printError("Input failed! Try again.");
            GeneralUtils::resetInputStream();
            continue;
        }
        
        config.setYtdlpIsExecutable(isExecutableChar == 'y');
        break;
    }
    GeneralUtils::printLog("Updated yt-dlp file information.");
    
    config.printConfig();
}

void promptSessionMode(Session& session) {
    char sessionMode {};
    GeneralUtils::askInput("Would you like to enter interactive mode (I)[default] or continuous mode (F)?", sessionMode, true);
    session.sessionMode = sessionMode != 'f' ? Session::SessionMode::INTERACTIVE : Session::SessionMode::FAST;
    session.printSession();
}

/**
 * The main functions
 */

int runYtdlp(Session& currentSession) {
    if (!GeneralUtils::isValidFile(currentSession.loadedConfig.getYtdlpPath()))
        promptValidYtdlpPath(currentSession.loadedConfig);

    promptSessionMode(currentSession);

    return 0;
}

int main() {
    // std::system("python3 yt-dlp --version");
    try
    {
        LoadedConfig config {LoadedConfig::loadConfig(ConfigUtils::configPath)};
        Session currentSession {config};
        int exitCode {runYtdlp(currentSession)};
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        throw;
    }
    
    return 0;
}