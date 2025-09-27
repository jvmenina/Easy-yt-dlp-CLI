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
#include <regex>

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
    const std::string noConfigMsg {"<< OPTION NOT YET CONFIGURED >>"};
    const std::regex linkRegex {"[-a-zA-Z0-9@:%._\\+~#=]{1,256}\\.[a-zA-Z0-9()]{1,6}\\b[-a-zA-Z0-9()@:%_\\+.~#?&\\/=]\\S*$"};
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

    std::pair<std::string, std::string> makeStringPair(const std::string& str1, const std::string& str2) {
        return std::move(std::pair<std::string, std::string>{str1, str2});
    }
    std::string pairListToString(
        const std::string& head_message, 
        const std::vector<std::pair<std::string, std::string>>& list,
        const std::string& default_message
    ) {
        std::string out {};
        out += ":: " + head_message + " ::\n";
        for (const std::pair<std::string, std::string> pair : list) {
            out += "\t|--> " + (!pair.first.empty() ? pair.first : default_message) + " = " + (!pair.second.empty() ? pair.second : default_message) + "\n";
        }
        out += "\t|-------------------------";
        return std::move(out);
    }

    bool isValidFile(const std::string& path) {
        return std::filesystem::is_regular_file(path);
    }

    bool isValidDirectory(const std::string& path) {
        return std::filesystem::is_directory(path);
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

    void stringStrip(std::string& str) {
        const auto whitespaces {" \t\n"};

        const auto strBegin = str.find_first_not_of(whitespaces);
        if (strBegin == std::string::npos) {
            str = "";
            return;
        }

        const auto strEnd = str.find_last_not_of(whitespaces);
        const auto substrSize = strEnd - strBegin + 1;
        str = std::move(str.substr(strBegin, substrSize));
    }

    enum class InputPostProcess {
        NONE,
        TO_LOWER,
        STRIP,
    };

    void askInput(const std::string& prompt, std::string& str, const std::vector<InputPostProcess>& postProcessList = {}) {
        printPrompt(prompt);
        std::getline(std::cin, str);
        for (InputPostProcess i : postProcessList) {
            switch (i) {
                case InputPostProcess::TO_LOWER:
                    stringToLowerInPlace(str);
                    break;
                case InputPostProcess::STRIP:
                    stringStrip(str);
                    break;
                default:
                    printError("Unexpected non-fatal error: Unhandled input post-processing request.");
                    break;
            }
        }
        std::cin.clear();
    }

    void askInput(const std::string& prompt, char& ch, bool toLower = false) {
        printPrompt(prompt);
        std::cin.get(ch);
        if (toLower) ch = std::tolower(static_cast<unsigned char>(ch));
        if (ch != '\n') 
            resetInputStream();
        else 
            std::cin.clear();
    }

    void askInputWithCheck(
        std::string& dest, 
        const std::string& prompt,
        bool (*const validatorFcn)(const std::string&),
        const std::string& messageIfInvalid,
        const std::string& messageIfEmpty,
        const std::vector<InputPostProcess>& postProcessList = {}
    ) {
        while (true) {
            GeneralUtils:;askInput(prompt, dest, postProcessList);
            if (std::cin.fail()) {
                GeneralUtils::printError("Input failed!");
                GeneralUtils::resetInputStream();
                continue;
            }
            if (!messageIfEmpty.empty() && dest.empty()) {
                GeneralUtils::printError(messageIfEmpty);
                continue;
            }
            if (!messageIfInvalid.empty() && validatorFcn(dest)) return;
            GeneralUtils::printError(messageIfInvalid);
        }
    }

    void askInputWithCheck(
        char& dest, 
        const std::string& prompt,
        bool (*const validatorFcn)(char&),
        const std::string& messageIfInvalid,
        bool toLower = false
    ) {
        while (true) {
            GeneralUtils:;askInput(prompt, dest, toLower);
            if (std::cin.fail()) {
                GeneralUtils::printError("Input failed!");
                GeneralUtils::resetInputStream();
                continue;
            }
            if (!messageIfInvalid.empty() && validatorFcn(dest)) return;
            GeneralUtils::printError(messageIfInvalid);
        }
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
            if (key.empty() || value.empty()) continue;
            
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
        GeneralUtils::printLog("Updated yt-dlp path.");
        return true;
    }

    bool ytdlpIsExecutable() {return this->m_ytdlpIsExecutable;}
    void setYtdlpIsExecutable(bool newVal) {
        this->m_ytdlpIsExecutable = newVal;
        GeneralUtils::printLog("Updated yt-dlp file information.");
    }

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
        out << GeneralUtils::pairListToString(
            "LOADED CONFIGURATION",
            std::vector<std::pair<std::string, std::string>> {
                GeneralUtils::makeStringPair(ConfigUtils::configKeywordStrings[0], config.m_ytdlpPath),
                GeneralUtils::makeStringPair(ConfigUtils::configKeywordStrings[1], (config.m_ytdlpIsExecutable ? "true" : "false")),
            },
            Constants::noConfigMsg
        );
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

    void setSessionMode(char sessionMode) {
        switch (sessionMode) {
            case 'f':
                this->sessionMode = SessionMode::FAST;
                break;
            default:
                this->sessionMode = SessionMode::INTERACTIVE;
                break;
        }
        GeneralUtils::printLog("Updated session mode.");
    }

    bool setInputLink(const std::string& inputLink) {
        std::smatch match {};
        if (!std::regex_search(inputLink, match, Constants::linkRegex))
            return false;
        this->inputLink = inputLink;
        GeneralUtils::printLog("Updated link.");
        return true;
    }

    void setOutputDirectory(const std::string& outputDirectory) {
        this->outputDirectory = outputDirectory;
        GeneralUtils::printLog("Updated output directory.");

        // if (!GeneralUtils::isValidDirectory(outputDirectory)) return false;
        // this->outputDirectory = outputDirectory;
        // GeneralUtils::printLog("Updated output directory.");
        // return true;
    }

    void printSession() {
        GeneralUtils::printLog(GeneralUtils::toString(*this));
    }

    /**
     * Externally defined methods
     */

    friend std::ostream& operator<<(std::ostream& out, const Session& session) {
        out << GeneralUtils::pairListToString(
            "CURRENT SESSION INFORMATION",
            std::vector<std::pair<std::string, std::string>> {
                GeneralUtils::makeStringPair("Mode", GeneralUtils::toString(session.sessionMode)),
                GeneralUtils::makeStringPair("Input Link", session.inputLink),
                GeneralUtils::makeStringPair("Output Directory", session.outputDirectory),
                GeneralUtils::makeStringPair("Output Filename", session.outputFilename),
                GeneralUtils::makeStringPair("Download Mode", GeneralUtils::toString(session.downloadMode)),
                GeneralUtils::makeStringPair("Download Options", session.downloadModeOptions),
            },
            Constants::noConfigMsg
        );
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
                out << Constants::noConfigMsg;
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
                out << Constants::noConfigMsg;
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
        GeneralUtils::askInput("Input new path:", newPath);
        if (std::cin.fail()) {
            GeneralUtils::printError("Input failed!");
            GeneralUtils::resetInputStream();
            continue;
        }
        if (newPath.length() < PATH_MAX && config.setYtdlpPath(newPath)) break;
        GeneralUtils::printError("Invalid path!");
    }
    
    char isExecutableChar {};
    while (true) {
        GeneralUtils::askInput("Is executable (Y / N[default])?", isExecutableChar, true);
        if (std::cin.fail()) {
            GeneralUtils::printError("Input failed!");
            GeneralUtils::resetInputStream();
            continue;
        }
        
        config.setYtdlpIsExecutable(isExecutableChar == 'y');
        break;
    }
    
    config.printConfig();
}

void promptSessionMode(Session& session) {
    char sessionMode {};
    GeneralUtils::askInput("Would you like to enter interactive mode (I)[default] or fast mode (F)?", sessionMode, true);
    if (std::cin.fail()) {
        GeneralUtils::printError("Input failed! Setting to default.");
        GeneralUtils::resetInputStream();
        sessionMode = '\n';
    }
    session.setSessionMode(sessionMode);
}

void promptInputLink(Session& session) {
    std::string inputLink {};
    while (true) {
        GeneralUtils::askInput("Input link:", inputLink, {GeneralUtils::InputPostProcess::STRIP});
        if (std::cin.fail()) {
            GeneralUtils::printError("Input failed!");
            GeneralUtils::resetInputStream();
            continue;
        }

        if (inputLink.empty()) {
            GeneralUtils::printError("Empty link!");
            continue;
        }
        if (session.setInputLink(inputLink)) break;
        GeneralUtils::printError("Invalid link!");
    }
    session.printSession();
}

// void promptOutput(Session& session) {
//     std::string outputDirectory {};
//     while (true) {
//         GeneralUtils::askInput("Input target directory:", outputDirectory, {GeneralUtils::InputPostProcess::STRIP});
//         if (std::cin.fail()) {
//             GeneralUtils::printError("Input failed!");
//             GeneralUtils::resetInputStream();
//             continue;
//         }
//         if (outputDirectory.empty()) {
//             GeneralUtils::printError("Empty path!");
//             continue;
//         }
//         if (session.setOutputDirectory(outputDirectory)) break;
//         GeneralUtils::printError("Invalid path!");
//     }
//     session.printSession();
// }
void promptOutput(Session& session) {
    std::string outputDirectory {};
    GeneralUtils::askInputWithCheck(
        outputDirectory,
        "Input target directory:",
        &GeneralUtils::isValidDirectory,
        "Invalid path!",
        "Empty path!",
        {GeneralUtils::InputPostProcess::STRIP}
    );
    session.setOutputDirectory(outputDirectory);
    // while (true) {
    //     GeneralUtils::askInput("Input target directory:", outputDirectory, {GeneralUtils::InputPostProcess::STRIP});
    //     if (std::cin.fail()) {
    //         GeneralUtils::printError("Input failed!");
    //         GeneralUtils::resetInputStream();
    //         continue;
    //     }
    //     if (outputDirectory.empty()) {
    //         GeneralUtils::printError("Empty path!");
    //         continue;
    //     }
    //     if (session.setOutputDirectory(outputDirectory)) break;
    //     GeneralUtils::printError("Invalid path!");
    // }
    session.printSession();
}

/**
 * The main functions
 */

int runYtdlp(Session& currentSession) {
    if (!GeneralUtils::isValidFile(currentSession.loadedConfig.getYtdlpPath()))
        promptValidYtdlpPath(currentSession.loadedConfig);

    promptSessionMode(currentSession);
    promptInputLink(currentSession);
    promptOutput(currentSession);

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