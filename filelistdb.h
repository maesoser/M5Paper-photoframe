#include <SdFat.h>
#define SD_FAT_TYPE 1

class FileListDB {
private:
    const char *storageFileName;  // File to store file list data
    const char *folderPath;  // File to store file list data

    bool has_suffix(const std::string &str, const std::string &suffix) {
      return str.size() >= suffix.size() &&
            str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool is_jpg(const std::string &filename) {
      return (has_suffix(filename, ".jpg") || has_suffix(filename, ".jpeg"));
    }

public:
    // Constructor: takes the file name where data will be stored
    FileListDB(const char *fileName, const char *folder) {
        storageFileName = fileName;
        folderPath = folder;
    }

    String getStorageFileName() {
        return String(storageFileName);
    }
    
    // Method to save the number of files and their names to a file
    void saveFileList() {
        File folder = SD.open(folderPath);
        if (!folder || !folder.isDirectory()) {
            Serial.println("Invalid folder path.");
            return;
        }

        File dataFile = SD.open(storageFileName, FILE_WRITE);
        if (!dataFile) {
            Serial.println("Error opening storage file.");
            return;
        }

        int fileCount = 0;
        String fileNames = "";

        // Iterate through all files in the folder
        File file = folder.openNextFile();
        while (file) {
            if (!file.isDirectory() && is_jpg(file.name())) {
                fileCount++;
                fileNames += file.name();
                fileNames += '\n';  // Newline to separate file names
            }
            file.close();
            file = folder.openNextFile();
        }

        // Write the file count and file names to the storage file
        dataFile.println(fileCount);
        dataFile.print(fileNames);

        Serial.println("File list saved successfully.");
        dataFile.close();
    }

    // Method to retrieve the number of files stored
    int getFileCount() {
        File dataFile = SD.open(storageFileName);
        if (!dataFile) {
            Serial.println("Error opening storage file.");
            return -1;
        }

        String line = dataFile.readStringUntil('\n');
        dataFile.close();
        return line.toInt();
    }
    // Method to retrieve a random file name from the db
    String getRandomFileName(){
        int n_files = this.getFileCount();
        int r_file = random(0, n_files);
        return this.getFileName(r_file);
    }

    // Method to retrieve a file name by index
    String getFileName(int index) {
        File dataFile = SD.open(storageFileName);
        if (!dataFile) {
            Serial.println("Error opening storage file.");
            return "";
        }
        // Skip the first line (file count)
        dataFile.readStringUntil('\n');
        // Read file names line by line
        for (int i = 0; i < index; i++) {
            if (!dataFile.readStringUntil('\n').length()) {
                Serial.println("Index out of bounds.");
                dataFile.close();
                return "";
            }
        }
        String fileName = dataFile.readStringUntil('\n');
        dataFile.close();
        return fileName;
    }
};
