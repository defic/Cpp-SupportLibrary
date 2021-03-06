/**
 * @file FileSystem.h
 * @brief Contains FileSystem I/O functions.
 * @author Daniel Giritzer
 * @copyright "THE BEER-WARE LICENSE" (Revision 42):
 * <giri@nwrk.biz> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return Daniel Giritzer
 */
#ifndef SUPPORTLIB_FILESYSTEM_H
#define SUPPORTLIB_FILESYSTEM_H

#include "Exception.h"
#include <vector>
#include <fstream>
#include <filesystem>
#include <future>
#include <iostream>

namespace giri {

    /**
     * @brief Namespace containing FileSystem I/O commands.
     */
    namespace FileSystem {

        /**
         * @brief Exception to be thrown on FileSystem errors.
         */
        class FileSystemException final : public ExceptionBase
        {
        public:
            FileSystemException(const std::string &msg) : ExceptionBase(msg) {}; 
            using SPtr = std::shared_ptr<FileSystemException>;
            using UPtr = std::unique_ptr<FileSystemException>;
            using WPtr = std::weak_ptr<FileSystemException>;
        };

        /**
         * Loads blob data from the harddisk. Throws FileSystemException
         * on error.
         * @param file Filepath to load data from.
         */
        inline std::vector<char> LoadFile(const std::filesystem::path& file) {
            if(!std::filesystem::exists(file))
                throw FileSystemException("File does not exist: " + file.string());

            std::ifstream in;
            in.open(file, std::ios::in | std::ios::binary);
            if (!in.good())
                throw FileSystemException("Could not open file: " + file.string());

            in.seekg(0, in.end); size_t len = in.tellg(); in.seekg(0, in.beg);
            std::vector<char> b(len);
            in.read((char*)&b[0], len);
            in.close();
            return b;
        }

        /**
         * Writes data to the harddisk. If given file does not
         * exist it will be created. If file does already exist it will
         * be overridden.
         * @param file Filepath to write data to.
         * @param data Data to write.
         * @returns true on success, false on failure.
         */
        inline bool WriteFile(const std::filesystem::path& file, const std::vector<char>& data)
        {
            std::ofstream out;
            out.open(file, std::ios::out | std::ios::binary);
            if (!out.good())
                return false;
            out.write((char*)&data[0], data.size() * sizeof(char));
            out.close();
            return true;
        }
        /**
         * Searches for an executable within PATH.
         * @param executable Name of executable to search for.
         * @returns Path to executable if existent, emtpty string if not existent.
         */
        inline std::filesystem::path FindExecutableInPath(const std::string &executable)
        {
            std::string path = getenv("PATH");
        #if defined(_WIN32)
            char delim = ';';
        #else
            char delim = ':';
        #endif
            std::vector<std::string> tokens;
            std::string token;
            std::istringstream tokenStream(path);
            while (std::getline(tokenStream, token, delim))
            {
                std::filesystem::path exec(token);
                exec.append(executable);
                if(std::filesystem::exists(exec))
                    return exec;
                exec.replace_extension(".exe");
                if(std::filesystem::exists(exec))
                    return exec;
            }
            return "";
        }
        /**
         * Executes a command synchronously.
         * @param cmd Command to execute.
         * @returns The output of the command after finishing execution.
         */
        inline std::string ExecuteSync(const std::string& cmd) {
            std::array<char, 128> buffer;
            std::string result;
        #if defined(_WIN32)
            std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
        #else
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        #endif
            if (!pipe) {
                throw FileSystemException("popen() failed!");
            }
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }
            return result;
        }
        /**
         * Executes a command asynchronously.
         * @param cmd Command to execute.
         * @param ostr Stream to write command output to. (defaults to std::cout) 
         * @returns Future object which can be used for synchronization.
         */
        inline std::future<void> ExecuteAsync(const std::string& cmd, std::ostream& ostr = std::cout) {
            auto hdl = std::async(std::launch::async, [&] { 
            #if defined(_WIN32)
                std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
            #else
                std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
            #endif
                if (!pipe) {
                    throw FileSystemException("popen() failed!");
                }
                std::array<char, 128> buffer;
                while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                    ostr << buffer.data();
                }
                return;
            });
            return hdl;
        }
    }
}
#endif //SUPPORTLIB_FILESYSTEM_H