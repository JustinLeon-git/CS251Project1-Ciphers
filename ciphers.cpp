#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "include/caesar_dec.h"
#include "include/caesar_enc.h"
#include "include/subst_dec.h"
#include "include/subst_enc.h"
#include "utils.h"

using namespace std;

// Initialize random number generator in .cpp file for ODR reasons
std::mt19937 Random::rng;

const string ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Function declarations go at the top of the file so we can call them
// anywhere in our program, such as in main or in other functions.
// Most other function declarations are in the included header
// files.

// When you add a new helper function, make sure to declare it up here!
// Print instructions for using the program.
void printMenu();

int main() {
  Random::seed(time(NULL));
  string command;
  vector<string> dictionary;

  // Reading in our dictionary for caesar decryption later
  ifstream dictFile("dictionary.txt");
  string word;
  while (getline(dictFile, word)) {
    dictionary.push_back(word);
  }

  // Reading in our quadgram scorer for our subst cipher decryption later
  ifstream quadgramFile("english_quadgrams.txt");
  string line;
  vector<string> quadgrams;
  vector<int> countNum;
  while (getline(quadgramFile, line)) {
    quadgrams.push_back(clean(line.substr(0,4)));
    countNum.push_back(stoi(line.substr(5, line.size() - 5)));
  }
  QuadgramScorer scorer(quadgrams, countNum);

  // Making a prototype to use in main
  vector<char> decryptSubstCipher(const QuadgramScorer& scorer, const string& ciphertext);

  cout << "Welcome to Ciphers!" << endl;
  cout << "-------------------" << endl;
  cout << endl;

  // Command loop
  do {
    // Prompts commands for user input
    printMenu();
    cout << endl << "Enter a command (case does not matter): ";

    // Use getline for all user input to avoid needing to handle
    // input buffer issues relating to using both >> and getline
    getline(cin, command);
    cout << endl;

    // Seeding random generator
    if (command == "R" || command == "r") {
      string seed_str;
      cout << "Enter a non-negative integer to seed the random number "
              "generator: ";
      getline(cin, seed_str);
      Random::seed(stoi(seed_str));

    // Caesar Encrypting
    } else if (command == "C" || command == "c") {
      caesarEncryptCommand();

    // Caesar Decrypting
    } else if (command == "D" || command == "d") {
      caesarDecryptCommand(dictionary);

    // Subst Encrypting
    } else if (command == "A" || command == "a") {
      applyRandSubstCipherCommand();

    // English Scorer
    } else if (command == "E" || command == "e") {
      computeEnglishnessCommand(scorer);

    // Subst Decrypting
    } else if (command == "S" || command == "s") {
      decryptSubstCipherCommand(scorer);

    // Subst Decrypting (File)
    } else if (command == "F" || command == "f") {
      string inputFilename, outputFilename;
      
      // Getting filenames from user input
      cout << "Input file name: ";
      getline(cin, inputFilename);
      cout << "Output file name: ";
      getline(cin, outputFilename);

      // Parsing our file's decrypting text
      ifstream encrypted(inputFilename);
      string encryptedStr, unencryptedStr;
      while (getline(encrypted, line)) {
        encryptedStr += line + "\n";
      }

      // Decrypting text
      vector<char> decryptKey = decryptSubstCipher(scorer, encryptedStr);
      unencryptedStr = applySubstCipher(decryptKey, encryptedStr);
      ofstream unencrypted(outputFilename);
      unencrypted << unencryptedStr;
      unencrypted.close();
    }

    cout << endl;

  } while (!(command == "x" || command == "X") && !cin.eof());

  return 0;
}

void printMenu() {
  cout << "Ciphers Menu" << endl;
  cout << "------------" << endl;
  cout << "C - Encrypt with Caesar Cipher" << endl;
  cout << "D - Decrypt Caesar Cipher" << endl;
  cout << "E - Compute English-ness Score" << endl;
  cout << "A - Apply Random Substitution Cipher" << endl;
  cout << "S - Decrypt Substitution Cipher from Console" << endl;
  cout << "F - Decrypt Substitution Cipher from File" << endl;
  cout << "R - Set Random Seed for Testing" << endl;
  cout << "X - Exit Program" << endl;
}

// "#pragma region" and "#pragma endregion" group related functions in this file
// to tell VSCode that these are "foldable". You might have noticed the little
// down arrow next to functions or loops, and that you can click it to collapse
// those bodies. This lets us do the same thing for arbitrary chunks!
#pragma region CaesarEnc

char rot(char c, int amount) {
  // Find the location of the char in the alphabet
  int charLocation = ALPHABET.find(c);

  // Finds the new character after rotation and accounts for wrapping if too large a jump
  c = ALPHABET.at((charLocation + amount) % 26);
  return c;
}

string rot(const string& line, int amount) {
  // Initializing our rotated string
  string encrypted;

  // Looping through every letter in our string
  for (char c : line) {
    // Checking if our character is a letter
    if (isalpha(c)) {
      // Running it through our char encrypter and adding it to encrypted string
      encrypted.push_back(rot((char)toupper(c), amount));

    // Otherwise we check if it is a space and add to our encrypted string
    } else if (isspace(c)) {
      encrypted += c;
    }
  }
  return encrypted;
}

void caesarEncryptCommand() {
  string inputText, rotation;

  // Getting input from user
  cout << "Text to encrypt: ";
  getline(cin, inputText);
  cout << "Amount to rotate: ";
  getline(cin, rotation);

  // Running encryption commands and printing output
  cout << rot(inputText, stoi(rotation)) << endl;
}

#pragma endregion CaesarEnc

#pragma region CaesarDec

void rot(vector<string>& strings, int amount) {
  // Looping through every word in our string vector
  for (string& word : strings) {
    word = rot(word, amount);
  }
}

string clean(const string& s) {
  string cleanString;
  for (char c : s) {
    // Checks if a char is a letter
    if (isalpha(c)) {
      cleanString.push_back(toupper(c));
    }
  }
  return cleanString;
}

vector<string> splitBySpaces(const string& s) {
  vector<string> splits;
  string cleanedStr;
  int lastIndex = 0;
  int newIndex = 0;
  // If our string contains nothing
  if (s.empty()) {
    return splits;
  
  // If our string has a single word
  } else if (s.find(' ') == string::npos) {
    splits.push_back(clean(s));
    return splits;
  }

  // Loops over every word until we reach the end of the string
  while (s.find(' ', lastIndex) != string::npos) {
    newIndex = s.find(' ', lastIndex);
    cleanedStr = clean(s.substr(lastIndex, newIndex - lastIndex));
    // Check if our cleaned string has a word
    if (!cleanedStr.empty()) {
      splits.push_back(cleanedStr);
    }
    lastIndex = newIndex + 1;
  }
  // Checks the last of the string
  cleanedStr = clean(s.substr(lastIndex, s.size() - lastIndex));
  if (!cleanedStr.empty()) {
    splits.push_back(cleanedStr);
  }
  return splits;
}

string joinWithSpaces(const vector<string>& words) {
  // Checking if our vector is empty
  if (words.empty()) { 
    return "";
  }
  // Adding in our first word
  string joint = words.at(0);

  // Looping through every word and adding a space before it and the next word
  for (size_t i = 1; i < words.size(); i++) {
    joint += ' ' + words.at(i);
  }
  return joint;
}

int numWordsIn(const vector<string>& words, const vector<string>& dict) {
  int count = 0;
  // Linear search of every word checking if it's in the dictionary
  for (string word : words) {
    for (string dictWord : dict) {
      if (word == dictWord) {
        count++;
        break;
      }
    }
  }
  return count;
}

void caesarDecryptCommand(const vector<string>& dict) {
  string encrypted;
  vector<string> encryptedList, decryptAttempt;
  int count;
  bool decryptFlag = false;
  cout << "Enter encrypted text: ";
  getline(cin, encrypted);
  // Cleaning up the encrypted text
  encryptedList = splitBySpaces(encrypted);
  
  // Looping through every possible rotation
  for (int i = 0; i < 26; i++) {
    decryptAttempt = encryptedList;
    // Rotate our decryption
    rot(decryptAttempt, i);
    // Check against dictionary
    count = numWordsIn(decryptAttempt, dict);

    // Valid decryptions printed
    if (count > decryptAttempt.size() / 2) {
      decryptFlag = true;
      cout << joinWithSpaces(decryptAttempt) << endl;
    }
  }
  // Checking if we found any decryptions
  if (!decryptFlag) {
    cout << "No good decryptions found" << endl;
  }
}

#pragma endregion CaesarDec

#pragma region SubstEnc

string applySubstCipher(const vector<char>& cipher, const string& s) {
  string encrypted = s;

  // Loop through entire unencrypted string
  for (size_t i = 0; i < s.size(); i++) {
    // Checks if its a letter
    if (isalpha(s.at(i))) {
      // Switches the letter with the encrypted alias
      encrypted.at(i) = cipher.at(ALPHABET.find(toupper(s.at(i))));
    }
  }
  return encrypted;
}

void applyRandSubstCipherCommand() {
  string inputText;
  cout << "Text to encrypt: ";
  getline(cin, inputText);
  cout << "Encrypted text: " << applySubstCipher(genRandomSubstCipher(), inputText);
}

#pragma endregion SubstEnc

#pragma region SubstDec

double scoreString(const QuadgramScorer& scorer, const string& s) {
  double score = 0.0;
  string cleaned;
  // Removing all non-letters
  for (char c : s) {
    if (isalpha(c)) {
      cleaned.push_back(toupper(c));
    }
  }

  // Edge case for our indexing later
  if (cleaned.size() < 4) {
    return 0.0;
  }

  // Loops through our letters for quadgrams
  for (size_t i = 0; i <= cleaned.size() - 4; i++) {
    score += scorer.getScore(cleaned.substr(i, 4));
  }
  return score;
}

void computeEnglishnessCommand(const QuadgramScorer& scorer) {
  string inputText;
  cout << "Enter text to score: ";
  getline(cin, inputText);
  cout << "Score: " << scoreString(scorer, inputText) << endl;
}

vector<char> decryptSubstCipher(const QuadgramScorer& scorer, const string& ciphertext) {
  vector<char> bestKey;
  int randInt1, randInt2;
  double score, scoreSwap, bestScore, curScore;
  char tempChar;

  // Loop through the climbing algorithm to find best maximum
  for (int i = 0; i < 55; i++) {
    vector<char> key = genRandomSubstCipher();
    // Go through 1000 different keys to find good key
    for (int i = 0; i < 1700; i++) {
      vector<char> keySwap = key;

      // Getting random integers for key swap
      randInt1 = Random::randInt(25);
      randInt2 = Random::randInt(25);
      while (randInt1 == randInt2) {
        randInt2 = Random::randInt(25);
      }

      // Swapping keySwap vals
      tempChar = keySwap.at(randInt1);
      keySwap.at(randInt1) = keySwap.at(randInt2);
      keySwap.at(randInt2) = tempChar;

      // Scoring decrypted string and determining which key is better
      score = scoreString(scorer, applySubstCipher(key, ciphertext));
      scoreSwap = scoreString(scorer, applySubstCipher(keySwap, ciphertext));
      if (scoreSwap > score) {
        key = keySwap;
      }
    }
    // Now we check if we reached a better local maximum
    curScore = scoreString(scorer, applySubstCipher(key, ciphertext));
    if (i == 0 || curScore > bestScore) {
      bestKey = key;
      bestScore = curScore;
    }
  }
  return bestKey;
}

void decryptSubstCipherCommand(const QuadgramScorer& scorer) {
  string inputText;

  // Getting and cleansing user input
  cout << "Input encrypted text: ";
  getline(cin, inputText);
  for (char& c : inputText) {
    if (isalpha(c)) {
      c = toupper(c);
    }
  }

  // Now we decrypt our input
  vector<char> key = decryptSubstCipher(scorer, inputText);
  cout << "Decrypted text: " << applySubstCipher(key, inputText) << endl;
}

#pragma endregion SubstDec
