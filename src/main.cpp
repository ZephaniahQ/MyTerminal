#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstring>
#include <vector>
#include<pty.h>
#include<unistd.h>
#include<sys/wait.h>
#include<fcntl.h>

class Console{

    public:

        static const int spacer = 10;

        std::vector<std::string> history; 
        bool newLineTriggered;

        std::string inputLine; 
        std::string ouput;
        std::string promptStr;
        char buffer[256];

        sf::Text m_text;
        sf::Text prompt;
        sf::Text cursor;
        sf::Font font;
        sf::Clock cursorClock;
        bool cursorVisible;

        float m_textSize;
        float m_height;

        Console(std::string fontfile) : m_height(0), cursorVisible(false)
    {
        loadFont(fontfile);
        promptStr = "myTerm_> ";
        prompt.setString(promptStr);
        prompt.setFont(font);
        prompt.setOrigin(0,0);
        prompt.setPosition(0,5);
        prompt.setCharacterSize(24);
        prompt.setFillColor(sf::Color::Green);

        cursor.setString("_");
        cursor.setFont(font);
        cursor.setCharacterSize(24);
        cursor.setFillColor(sf::Color::White);

        buffer[0] = '\0';
        newLineTriggered = false;
    }

        void loadFont(std::string filename)
        {
            if(!font.loadFromFile(filename))
                std::cout << "font load failed" << std::endl;
            else std::cout << "Successfull loaded font" << std::endl;
            m_text.setFont(font);
            m_text.setCharacterSize(24);
            m_text.setFillColor(sf::Color::White);
        }

        bool separatePrompt(std::string& line)
        {
            int tilde = line.find("~");
            int dollar = line.find("$");

            if(tilde >= 0 && dollar >=0)
            {
                std::string pwd = line.substr(tilde, dollar-tilde + 1);
                promptStr.clear();
                promptStr = "myTerm_>"+pwd + " ";
                prompt.setString(promptStr);
                if(dollar == line.size()-2)
                    return false;
                line = line.substr(dollar+1, line.size()-dollar);
            }
            return true;
        }

        void updateAndDraw(sf::RenderWindow& window)
        {
            int promptWidth = prompt.getLocalBounds().width;
            int promptHeight = prompt.getLocalBounds().height;

            m_height = spacer;

            bool cleared = history.size() != 0 && history[0].find("clear") != std::string::npos;
            if(!cleared)
            {
                for(int i = 0; i<history.size(); i++)
                {
                    if(!separatePrompt(history[i]))
                        continue;
                    prompt.setPosition(spacer, m_height);
                    m_text.setOrigin(0,0);
                    m_text.setPosition(promptWidth + spacer, m_height);
                    m_text.setString(history[i]);

                    window.draw(prompt);
                    window.draw(m_text);

                    m_height += promptHeight + spacer;
                }
            }
            else history.pop_back();
            
            m_text.setString(inputLine);
            m_text.setOrigin(0,0);
            m_text.setPosition(promptWidth + spacer, m_height);

            prompt.setPosition(spacer, m_height);
            window.draw(prompt);
            window.draw(m_text);

            updateCursorClock();

            if(cursorVisible)
            {
                int textWidth = m_text.getLocalBounds().width;
                cursor.setPosition(textWidth + promptWidth + spacer,m_height);
                window.draw(cursor);
            }
        }

        void addInputChar(char c)
        {
            inputLine += c;
        }

        void removeLastChar()
        {
            if(!inputLine.empty())
                inputLine.pop_back();
        }

        void triggerNewLine(int master_fd)
        {
            if(inputLine == "clear")
            {
                history.clear();
                history.shrink_to_fit();
            }

            inputLine += '\r';
            write(master_fd, inputLine.c_str(),inputLine.size());
        }

        std::string filterAnsi(std::string buffer)
        {
            std::string cleanOutput = "";

            for(int i = 0; i< buffer.size(); i++)
            {
                if(buffer[i] == '\x1B' && cleanOutput != inputLine + '\r')
                {
                    while(i<buffer.size() && buffer[i] != 'm') i++;
                    continue;
                }

                if(isprint(buffer[i]) || buffer[i] == '\n')
                {
                    cleanOutput += buffer[i];
                }
            }

            return cleanOutput;
        }

        std::string filterAnsi(char* buf, int size)
        {
            std::string cleanOutput = "";

            for(int i = 0; i< size; i++)
            {
                if(buffer[i] == '\x1B' && cleanOutput != inputLine + '\r')
                {
                    while(i<size && buffer[i] != 'm') i++;
                    continue;
                }

                if(isprint(buffer[i]) || buffer[i] == '\n')
                {
                    cleanOutput += buffer[i];
                    if(buffer[i] == '\n')
                        continue;
                }
            }

            return cleanOutput;
        }

        void getResult(int master_fd)
        {

            ssize_t bytesRead;
            std::string cleanOutput;

            while((bytesRead = read(master_fd, buffer, sizeof(buffer)-1)) > 0)
            {
                buffer[bytesRead] = '\0';
                cleanOutput = "";
                
                cleanOutput = filterAnsi(buffer, bytesRead);

                if(!cleanOutput.empty() && cleanOutput != inputLine && cleanOutput != inputLine + "\r")
                {
                    history.push_back(cleanOutput);
                }
                inputLine.clear();
            }

            if(bytesRead < 0 && errno != EAGAIN)
            {
                std::cerr << "Read error: " << strerror(errno) << std::endl;
            }


        }

        void updateCursorClock()
        {
            //handle cursorClock

            if(cursorClock.getElapsedTime().asSeconds() >= 0.5f)
            {
                cursorVisible = !cursorVisible;
                cursorClock.restart();
            }
        }
};

int main()
{
    int master_fd;
    int child_fd;

    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);
    fcntl(master_fd, F_SETFL, O_NONBLOCK);

    int sWidth = 1600;
    int sHeight = 900;

    if(pid < 0)
    {
        std::cerr << "forkpty failed!" << std::endl;
    }
    else if(pid == 0)
    {
        //execlp("/bin/bash", "bash","--norc", "-i", nullptr); //run bash
        execlp("/bin/bash", "bash","--norc", "-i", "-c", "PS1='\\w > ' bash", nullptr); //run bash
        std::cerr << "exec failed!" << std::endl;
        return 1;
    }
    else{

        //now this should only be running n parent process:
        //
        //testing a read and write: 

        std::string fontfile ="OpenSans-VariableFont_wdth,wght.ttf";
        Console console(fontfile); 

        //write(master_fd, "ls\r", 4);
        //read(master_fd, console.buffer, sizeof(console.buffer)-1);
        //std::cout << console.buffer << std::endl;

        sf::RenderWindow window(sf::VideoMode(sWidth, sHeight), "Terminal Emu");
        window.setFramerateLimit(60);
        sf::Clock clock = sf::Clock();

        while(window.isOpen()) {

            sf::Event event;
            while(window.pollEvent(event)) {

                if(event.type == sf::Event::Closed)
                    window.close();
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                {
                    window.close();
                    exit(0);
                }
                if(event.type == sf::Event::TextEntered) {
                    if(event.text.unicode < 128) {
                        char enteredChar = static_cast<char>(event.text.unicode);
                        if(enteredChar == '\b' && !console.inputLine.empty()) {
                            console.inputLine.pop_back(); //handle backspace
                        }
                        else if(enteredChar == '\r')
                        {
                            console.triggerNewLine(master_fd);
                        }
                        else if(enteredChar != '\b') {
                            console.inputLine += enteredChar; //add to input
                        }
                    }
                }
            }

            window.clear(sf::Color::Black);
            console.updateAndDraw(window);
            console.getResult(master_fd);
            window.display();
        }

        int status;
        waitpid(pid, &status, 0);
        return 0;
    }
}
