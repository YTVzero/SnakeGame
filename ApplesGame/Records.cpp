#include "Records.h"
#include <fstream>

// Аналог SerializeGame из гайда
bool SerializeGame(const RecordsTable& table)
{
    std::ofstream file(RECORDS_FILE);
    if (file.is_open())
    {
        for (const auto& record : table)
            file << record.first << " " << record.second << std::endl;
        file.close();
        return true;
    }
    return false;
}

// Аналог DeserializeGame из гайда
bool DeserializeGame(RecordsTable& table)
{
    std::ifstream file(RECORDS_FILE);
    if (file.is_open())
    {
        std::string name;
        int score;
        while (file >> name >> score)
            table[name] = score;
        file.close();
        return true;
    }
    return false;
}
