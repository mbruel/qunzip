#include <iostream>
#include <cstring>
#include <zip.h>
#include <QDir>
#include <QFile>

#define BUFF_SIZE 1024

int main(int argc, char *argv[])
{
    // 0.: check syntax
    if (argc < 2 || argc > 3){
        std::cout << "Syntax: " << argv[0] << " <zip file> <password>?" << std::endl;
        return 1;
    }

    char *archive = argv[1];
    std::cout << "Test libzip to unzip archive: " << archive << std::endl;


    // 1.: open zip
    int err = 0;
    struct zip *za = zip_open(archive, 0, &err);
    if (!za)
    {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        std::cerr << "Can't open zip file '" << archive << "' : " << zip_error_strerror(&error)
                  << " (err_code: " << err << ")" << std::endl;
        zip_error_fini(&error);
        return 2;
    }


    // 2.: set password (if provided)
    if (argc == 3 && zip_set_default_password(za, argv[2]) != 0)
    {
        std::cerr << "Error setting zip file password '"<< argv[2] << "'..." << std::endl;
        return 3;
    }


    // 3.: unzip files
    struct zip_stat sb;
    zip_uint64_t nbFiles = static_cast<zip_uint64_t>(zip_get_num_entries(za, 0));
    char buf[BUFF_SIZE];
    for (zip_uint64_t i = 0; i < nbFiles; ++i) {
        if (zip_stat_index(za, i, 0, &sb) == 0) {
            std::cout << "\t- " << sb.name << ", size: "<< sb.size << std::endl;

            size_t len = strlen(sb.name);
            if (sb.name[len - 1] == '/')
            {
                QDir dir(sb.name);
                if (dir.exists())
                    std::cerr << "Warning: the folder already exists..." << std::endl;
                else
                {
                    if (!dir.mkpath("."))
                        std::cerr << "Error creating folder " << sb.name << std::endl;
                }
            }
            else
            {
                struct zip_file *zf = zip_fopen_index(za, i, 0);
                if (zf)
                {
                    QFile file(sb.name);
                    if (!file.open(QIODevice::WriteOnly))
                        std::cerr << "Error creating decrypted file.." << std::endl;
                    else
                    {
                        zip_uint64_t sum = 0;
                        while (sum != sb.size) {
                            zip_int64_t len = zip_fread(zf, static_cast<void*>(buf), BUFF_SIZE);
                            if (len > 0) {
                                file.write(buf, len);
                                sum += static_cast<zip_uint64_t>(len);
                            }
                            else
                            {
                                std::cerr << "Error reading encrypted file..." << std::endl;
                                break;
                            }
                        }
                        file.close();
                    }
                    zip_fclose(zf);
                }
                else
                    std::cerr << "Error opening encrypted file.. (wrong password?)" << std::endl;
            }
        }
        else
            std::cerr << "Error getting information on file with index: " << i << std::endl;
    }


    // 4.: close zip
    if (zip_close(za) != 0)
    {
        std::cerr << "Error closing zip archive...\n";
        return 4;
    }

    return 0;
}
