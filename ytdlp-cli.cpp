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
 * Useful aliases
 */

using StringPair = std::pair<std::string, std::string>;
using PairVector = std::vector<StringPair>;

namespace Constants {
    const std::string noConfigMsg {"<< OPTION NOT YET CONFIGURED >>"};
    const std::regex linkRegex {"[-a-zA-Z0-9@:%._\\+~#=]{1,256}\\.[a-zA-Z0-9()]{1,6}\\b[-a-zA-Z0-9()@:%_\\+.~#?&\\/=]\\S*$"};
    const std::regex filenameRegex {"^[\\w\\-. ]+$"};
    const std::regex numberRegex {"^[1-9][0-9]+$"};
}

/**
 * Namespace for specialized constants and utility functions
 */

namespace Filename {
    enum class FormatTypes {
        NONE,
        TITLE_AND_ID,
        UPLOADER_AND_ID,
        WEBSITE_AND_ID,
        CUSTOM
    };

    struct Format {
        char id;
        std::string name;
        std::string format;

        Format(const char& id, const std::string& name, const std::string& format):
            id {id},
            name {name},
            format {format}
        {}
    };

    class FFTS {
        const std::map<FormatTypes, Format> typeToFormat;
        const std::map<char, FormatTypes> charToType;

        FFTS(const std::map<FormatTypes, Format>& names, const std::map<char, FormatTypes>& details)
            : typeToFormat {std::move(names)}
            , charToType {std::move(details)}
        {}
    
    public:
        static FFTS makeFFTS(std::initializer_list<std::pair<FormatTypes, Format>> list) {
            std::map<FormatTypes, Format> typeToFormat;
            std::map<char, FormatTypes> charToType;
            for (auto i : list) {
                typeToFormat.insert({i.first, i.second});
                charToType.insert({i.second.id, i.first});
            }
            return FFTS {typeToFormat, charToType};
        }

        FormatTypes charToFilenameFormatType(const char& ch) const {
            return charToType.at(ch);
        }

        Format getFormat(FormatTypes type) const {
            return typeToFormat.at(type);
        }

        std::vector<Format> getFormats() const {
            std::vector<Format> out {};
            for (auto i : typeToFormat) out.push_back(i.second);
            return std::move(out);
        }
    };

    const FFTS filenameFormats {FFTS::makeFFTS({
        {FormatTypes::TITLE_AND_ID, Format{'t', "Title and ID", "%(title)s - %(id)s.%(ext)s"}},
        {FormatTypes::UPLOADER_AND_ID, Format{'u', "Uploader and ID", "%(uploader)s - %(id)s.%(ext)s"}},
        {FormatTypes::WEBSITE_AND_ID, Format{'w', "Website and ID", "%(webpage_url_domain)s - %(id)s.%(ext)s"}},
        {FormatTypes::CUSTOM, Format{'c', "Custom", ".%(ext)s"}}
    })};
}

namespace Download {
    enum class Mode {
        AS_VIDEO,
        AS_AUDIO
    };

    enum class VideoOptions {
        BEST_ANY,
        BEST_MP4,
        CUSTOM
    };

    const std::string AudioOption {"-f \"bestaudio\" -x --audio-format mp3 --audio-quality 0"};
    const std::string VideoBestAnyOption {"-f \"bv*+ba/b\""};
    const std::string VideoBestMp4Option {"-f \"bv*[ext=mp4]+ba[ext=m4a] / b[ext=mp4]\""};
    const std::string VideoCustomOption {"-S \"res:"};

    Mode charToDownloadMode(const char& input) {
        switch (input) {
            case '2':
                return Mode::AS_AUDIO;
            default:
                return Mode::AS_VIDEO;
        }
    }

    VideoOptions charToVideoOption(const char& input) {
        switch (input) {
            case '2':
                return VideoOptions::BEST_MP4;
            case '3':
                return VideoOptions::CUSTOM;
            default:
                return VideoOptions::BEST_ANY;
        }
    }

    const std::string& getDownloadOptions(const Mode& mode, const VideoOptions& videoOption = VideoOptions::BEST_ANY) {
        if (mode == Mode::AS_AUDIO) {
            return AudioOption;
        } else if (videoOption == VideoOptions::BEST_MP4) {
            return VideoBestMp4Option;
        } else if (videoOption == VideoOptions::CUSTOM) {
            return VideoCustomOption;
        } else {
            return VideoBestAnyOption;
        }
    }

    std::ostream& operator<<(std::ostream& out, Download::Mode downloadMode) {
        switch (downloadMode) {
            case Download::Mode::AS_VIDEO:
                out << "AS VIDEO";
                break;
            case Download::Mode::AS_AUDIO:
                out << "AS AUDIO";
                break;
            default:
                out << Constants::noConfigMsg;
                break;
        }
        return out;
    }
}

namespace Config {
    const std::string configPath {"./ytdlp-cli.ini"};
    enum class configKeyword {
        YTDLP_PATH,
        YTDLP_IS_EXECUTABLE
    };

    struct ConfigOption {
        std::string option_name;
        std::string option_default_value;
        ConfigOption(const std::string& option_name, const std::string& option_default_value):
            option_name {option_name},
            option_default_value {option_default_value}
        {}
    };

    const std::map<configKeyword, ConfigOption> configOptions {
        {configKeyword::YTDLP_PATH, ConfigOption{"ytdlp_path", "./yt-dlp"}},
        {configKeyword::YTDLP_IS_EXECUTABLE, ConfigOption{"ytdlp_is_executable", "false"}},
    };

    void createDefaultConfigFile(const std::string& path) {
        std::ofstream configFile {path};
        std::string defaultConfigString {};
        for (std::pair<configKeyword, ConfigOption> c : configOptions) {
            defaultConfigString.append(c.second.option_name + "=" + c.second.option_name + "\n");
        }
        configFile << defaultConfigString << std::endl;
        configFile.close();
    }
}

/**
 * Namespace for general-purpose utility functions for convenience
 */
namespace Utils {
    

    namespace Printers {
        void printLog(const std::string& message) {
            std::cout << ">> (LOG) " + message << std::endl;
        }

        void printError(const std::string& message) {
            std::cout << ">> (ERROR) " + message << std::endl;
        }

        void printPrompt(const std::string& message) {
            std::cout << ">> [PROMPT] " + message + " " << std::flush;
        }
    }

    namespace Constructors {
        StringPair makeStringPair(const std::string& str1, const std::string& str2) {
            return std::move(StringPair{str1, str2});
        }
        std::string pairListToString(
            const std::string& head_message, 
            const PairVector& list,
            const std::string& default_message
        ) {
            std::string out {};
            out += ":: " + head_message + " ::\n";
            for (const StringPair pair : list) {
                out += "\t|--> " + (!pair.first.empty() ? pair.first : default_message) + " = " + (!pair.second.empty() ? pair.second : default_message) + "\n";
            }
            out += "\t|-------------------------";
            return std::move(out);
        }
    }

    namespace Validators {
        bool isValidFile(const std::string& path) {
            return path.length() < PATH_MAX && std::filesystem::is_regular_file(path);
        }

        bool isValidDirectory(const std::string& path) {
            return path.length() < PATH_MAX && std::filesystem::is_directory(path);
        }

        bool isValidLink(const std::string& link) {
            std::smatch match {};
            return std::regex_search(link, match, Constants::linkRegex);
        }

        bool isValidFilename(const std::string& filename) {
            std::smatch match {};
            return std::regex_search(filename, match, Constants::filenameRegex);
        }

        bool isNumber(const std::string& string) {
            std::smatch match {};
            return std::regex_search(string, match, Constants::numberRegex);
        }
    }

    namespace Transformers {
        template <typename T>
        std::string toString(const T& value)
        {
            std::ostringstream ss;
            ss << value;
            return ss.str();
        }

        char charToUpper (const char& ch) {
            return std::toupper(static_cast<unsigned char>(ch));
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
    }

    namespace Inputs {
        void resetInputStream() {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        enum class InputPostProcess {
            NONE,
            TO_LOWER,
            STRIP,
        };

        void askInput(const std::string& prompt, std::string& str, const std::vector<InputPostProcess>& postProcessList = {}) {
            Printers::printPrompt(prompt);
            std::getline(std::cin, str);
            for (InputPostProcess i : postProcessList) {
                switch (i) {
                    case InputPostProcess::TO_LOWER:
                        Transformers::stringToLowerInPlace(str);
                        break;
                    case InputPostProcess::STRIP:
                        Transformers::stringStrip(str);
                        break;
                    default:
                        Printers::printError("Unexpected non-fatal error: Unhandled input post-processing request.");
                        break;
                }
            }
            std::cin.clear();
        }

        void askInput(const std::string& prompt, char& ch, bool toLower = false) {
            Printers::printPrompt(prompt);
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
                    Printers::printError("Input failed!");
                    resetInputStream();
                    continue;
                }
                if (!messageIfEmpty.empty() && dest.empty()) {
                    Printers::printError(messageIfEmpty);
                    continue;
                }
                if (!validatorFcn) return;
                if (validatorFcn(dest)) return;
                Printers::printError(messageIfInvalid);
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
                    Printers::printError("Input failed!");
                    resetInputStream();
                    continue;
                }
                if (!validatorFcn) return;
                if (validatorFcn(dest)) return;
                Printers::printError(messageIfInvalid);
            }
        }
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
        Utils::Printers::printLog("Config loaded.");
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
            Config::createDefaultConfigFile(path);
            inputConfig.close();
            inputConfig = std::ifstream{path};
        }
        
        /**
         * Initialize constructor arguments using defaults
         */
        
        std::string ytdlpPath {Config::configOptions.at(Config::configKeyword::YTDLP_PATH).option_default_value};
        bool ytdlpIsExecutable {Config::configOptions.at(Config::configKeyword::YTDLP_IS_EXECUTABLE).option_default_value == "true"};

        /**
         * Use config file to update arguments
         */

        for (std::string line; std::getline(inputConfig, line);) {
            std::size_t delimIndex {line.find('=')};
            if (delimIndex == std::string::npos) continue;

            const std::string& key {line.substr(0,delimIndex)};
            const std::string& value {line.substr(delimIndex+1)};
            if (key.empty() || value.empty()) continue;
            
            if (key == Config::configOptions.at(Config::configKeyword::YTDLP_PATH).option_name)
                ytdlpPath = value;
            else if (key == Config::configOptions.at(Config::configKeyword::YTDLP_IS_EXECUTABLE).option_name)
                ytdlpIsExecutable = value == "true";
        }

        return LoadedConfig {ytdlpPath, ytdlpIsExecutable};
    }

    /**
     * Core methods
     */

    ~LoadedConfig() {Utils::Printers::printLog("Loaded configuration was destroyed.");}

    const std::string& getYtdlpPath() const {return this->m_ytdlpPath;}
    void setYtdlpPath(std::string& newPath) {
        this->m_ytdlpPath = newPath;
        Utils::Printers::printLog("Updated yt-dlp path.");
    }

    bool ytdlpIsExecutable() {return this->m_ytdlpIsExecutable;}
    void setYtdlpIsExecutable(bool newVal) {
        this->m_ytdlpIsExecutable = newVal;
        Utils::Printers::printLog("Updated yt-dlp file information.");
    }

    /**
     * General utility methods
     */

    void printConfig() {
        Utils::Printers::printLog(Utils::Transformers::toString(*this));
    }

    /**
     * Externally defined methods
     */

    friend std::ostream& operator<<(std::ostream& out, const LoadedConfig& config) {
        out << Utils::Constructors::pairListToString(
            "LOADED CONFIGURATION",
            PairVector {
                Utils::Constructors::makeStringPair(Config::configOptions.at(Config::configKeyword::YTDLP_PATH).option_name, config.m_ytdlpPath),
                Utils::Constructors::makeStringPair(Config::configOptions.at(Config::configKeyword::YTDLP_IS_EXECUTABLE).option_name, (config.m_ytdlpIsExecutable ? "true" : "false")),
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
    static bool unique;

    bool doUpdate {false};
    LoadedConfig& loadedConfig;
    std::string inputLink {};
    std::string outputDirectory {};
    Filename::FormatTypes outputFilenameFormatType {Filename::FormatTypes::NONE};
    std::string outputFilename {};
    Download::Mode downloadMode {Download::Mode::AS_VIDEO};
    std::string downloadModeOptions {};

    /**
     * Core methods
     */

    Session(LoadedConfig& config):
        loadedConfig {config}
    {
        if (!unique) throw std::runtime_error("Unexpected Error: Another Session has been initialized.");
        Utils::Printers::printLog("Connected loaded config to current session.");
        this->loadedConfig.printConfig();
        Utils::Printers::printLog("Session initialized.");
        unique = false;
    }

    ~Session() {
        Utils::Printers::printLog("Loaded session was destroyed.");
    }

    /**
     * General utility methods
     */

    void setInputLink(const std::string& inputLink) {
        this->inputLink = inputLink;
        Utils::Printers::printLog("Updated link.");
    }

    void setOutputDirectory(const std::string& outputDirectory) {
        this->outputDirectory = outputDirectory;
        Utils::Printers::printLog("Updated output directory.");
    }

    void setFilenameFormat(const char& format) {
        this->outputFilenameFormatType = Filename::filenameFormats.charToFilenameFormatType(format);
        this->outputFilename = Filename::filenameFormats.getFormat(this->outputFilenameFormatType).format;
        Utils::Printers::printLog("Updated output filename format.");
    }

    void setFilenameOnly(const std::string& filename) {
        this->outputFilename = filename + this->outputFilename;
        Utils::Printers::printLog("Updated output filename.");
    }

    void setDownloadMode(const char& mode) {
        this->downloadMode = Download::charToDownloadMode(mode);
        Utils::Printers::printLog("Updated download mode.");
    }

    void setDownloadOptions(const char& mode = 0) {
        if (this->downloadMode == Download::Mode::AS_AUDIO) {
            this->downloadModeOptions = Download::getDownloadOptions(this->downloadMode);
        } else {
            this->downloadModeOptions = Download::getDownloadOptions(this->downloadMode, Download::charToVideoOption(mode));
        }
        Utils::Printers::printLog("Updated download options.");
    }

    void setCustomDownloadOption(const std::string& options) {
        this->downloadModeOptions = Download::VideoCustomOption + options + "\"";
    }

    void printSession() {
        Utils::Printers::printLog(Utils::Transformers::toString(*this));
    }

    std::string getFullCommand() {
        std::string space {" "};
        if (doUpdate) return "python3" + space + loadedConfig.getYtdlpPath() + space + "--update";

        std::filesystem::path outDir {outputDirectory};
        outDir /= outputFilename;
        return "python3"
            + space + loadedConfig.getYtdlpPath()
            + space + downloadModeOptions
            + space + "-o" + space + "\"" + outDir.string() + "\""
            + space + "\"" + inputLink + "\""
            ;
    }

    void reset() {
        doUpdate = false;
        inputLink = "";
        outputFilenameFormatType = {Filename::FormatTypes::NONE};
        outputFilename = "";
        downloadMode = {Download::Mode::AS_VIDEO};
        downloadModeOptions = "";
    }

    /**
     * Externally defined methods
     */

    friend std::ostream& operator<<(std::ostream& out, const Session& session) {
        out << Utils::Constructors::pairListToString(
            "CURRENT SESSION INFORMATION",
            PairVector {
                Utils::Constructors::makeStringPair("Input Link", session.inputLink),
                Utils::Constructors::makeStringPair("Output Directory", session.outputDirectory),
                Utils::Constructors::makeStringPair("Output Filename", session.outputFilename),
                Utils::Constructors::makeStringPair("Download Mode", Utils::Transformers::toString(session.downloadMode)),
                Utils::Constructors::makeStringPair("Download Options", session.downloadModeOptions),
            },
            Constants::noConfigMsg
        );
        return out;
    }
};
bool Session::unique = true;

/**
 * General program flow functions
 */

void promptValidYtdlpPath(LoadedConfig &config) {
    Utils::Printers::printLog("Config-defined yt-dlp path is invalid.");
    
    std::string newPath {};
    Utils::Inputs::askInputWithCheck(
        newPath,
        "Input new path:",
        &Utils::Validators::isValidFile,
        "Invalid path!",
        "Empty path!",
        {Utils::Inputs::InputPostProcess::STRIP}
    );
    config.setYtdlpPath(newPath);
    
    char isExecutableChar {};
    Utils::Inputs::askInputWithCheck(
        isExecutableChar,
        "Is executable (Y / N[default])?",
        nullptr,
        "",
        true
    );
    config.setYtdlpIsExecutable(isExecutableChar == 'y');
    config.printConfig();
}

void promptUpdate(Session& session) {
    char update {};
    Utils::Inputs::askInputWithCheck(
        update,
        "Update? (Y / N[default]):",
        nullptr,
        "",
        true
    );
    session.doUpdate = update == 'y';
}

void promptInputLink(Session& session) {
    std::string inputLink {};
    Utils::Inputs::askInputWithCheck(
        inputLink,
        "Input link:",
        &Utils::Validators::isValidLink,
        "Invalid link!",
        "Empty link!",
        {Utils::Inputs::InputPostProcess::STRIP}
    );
    session.setInputLink(inputLink);
    session.printSession();
}

void promptOutputDirectory(Session& session) {
    if (session.outputDirectory != "") {
        char changeDirectory {};
        Utils::Inputs::askInputWithCheck(
            changeDirectory,
            "Reuse output directory? (Y[default] / N):",
            nullptr,
            "",
            true
        );
        if (changeDirectory != 'n') {
            Utils::Printers::printLog("Keeping previous directory.");
            return;
        }
    }
    std::string outputDirectory {};
    Utils::Inputs::askInputWithCheck(
        outputDirectory,
        "Output target directory:",
        &Utils::Validators::isValidDirectory,
        "Invalid path!",
        "Empty path!",
        {Utils::Inputs::InputPostProcess::STRIP}
    );
    session.setOutputDirectory(outputDirectory);
    session.printSession();
}

void promptFilename(Session& session) {
    char outputFilenameFormat {};
    std::string prompt {"Output filename format ("};
    bool firstPrint {true};
    for (const Filename::Format& i : Filename::filenameFormats.getFormats()) {
        if (!firstPrint) prompt += " | ";
        firstPrint = false;
        prompt += "[" + std::string{i.id};
        prompt += "]" + i.name;
    }
    prompt += "):";
    Utils::Inputs::askInputWithCheck(
        outputFilenameFormat,
        prompt,
        nullptr,
        "",
        true
    );
    session.setFilenameFormat(outputFilenameFormat);

    std::string filename {};
    if (session.outputFilenameFormatType == Filename::FormatTypes::CUSTOM) {
        Utils::Inputs::askInputWithCheck(
            filename,
            "Output custom filename:",
            &Utils::Validators::isValidFilename,
            "Invalid filename!",
            "Empty filename!",
            {Utils::Inputs::InputPostProcess::STRIP}
        );
        session.setFilenameOnly(filename);
    }
    session.printSession();
}

void promptDownloadMode(Session& session) {
    char mode {};
    Utils::Inputs::askInputWithCheck(
        mode,
        "Input download mode ([1] As Video (DEFAULT) | [2] As Audio):",
        nullptr,
        ""
    );
    session.setDownloadMode(mode);

    if (session.downloadMode == Download::Mode::AS_VIDEO) {
        Utils::Inputs::askInputWithCheck(
            mode,
            "Input download mode ([1] Any Best (DEFAULT) | [2] Best MP4 | [3] Custom ):",
            nullptr,
            ""
        );
        session.setDownloadOptions(mode);
    } else {
        session.setDownloadOptions();
    }

    if (session.downloadModeOptions == Download::VideoCustomOption) {
        std::string options {};
        Utils::Inputs::askInputWithCheck(
            options,
            "Custom resolution (Example: 720):",
            &Utils::Validators::isNumber,
            "Invalid resolution!",
            "Empty resolution!",
            {Utils::Inputs::InputPostProcess::STRIP}
        );
        session.setCustomDownloadOption(options);
    }
    session.printSession();
}

void confirmDownload(const std::string& command) {
    char confirm {};
    Utils::Inputs::askInputWithCheck(
        confirm,
        "Confirm download? (Y[default] / N):",
        nullptr,
        "",
        true
    );
    if (confirm == 'n') {
        Utils::Printers::printLog("Cancelled download.");
        return;
    }
    Utils::Printers::printLog("Confirmed download.");
    std::system(command.c_str());
}

/**
 * The main functions
 */

int runYtdlp(Session& currentSession) {
    if (!Utils::Validators::isValidFile(currentSession.loadedConfig.getYtdlpPath()))
        promptValidYtdlpPath(currentSession.loadedConfig);
    
    promptUpdate(currentSession);
    if (currentSession.doUpdate) {
        Utils::Printers::printLog("Compiled the following output...");
        const std::string fullCommand = currentSession.getFullCommand();
        Utils::Printers::printLog(fullCommand);
        confirmDownload(fullCommand);
        currentSession.doUpdate = false;
    }

    while (true) {
        promptInputLink(currentSession);
        promptOutputDirectory(currentSession);
        promptFilename(currentSession);
        promptDownloadMode(currentSession);
        Utils::Printers::printLog("Compiled the following output...");
        const std::string fullCommand = currentSession.getFullCommand();
        Utils::Printers::printLog(fullCommand);

        confirmDownload(fullCommand);
        currentSession.reset();
    }

    return 0;
}

int main() {
    try
    {
        std::system("clear");
        LoadedConfig config {LoadedConfig::loadConfig(Config::configPath)};
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