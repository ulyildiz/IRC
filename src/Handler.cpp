#include "../include/IRC_Protocol.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <unistd.h>

// Numeric reply sabitleri
#define ERR_NEEDMOREPARAMS 461
#define ERR_PASSWDMISMATCH 464
#define ERR_NOTREGISTERED 451
#define RPL_WELCOME 001

// PASS
void IRC_Protocol::handlePASS(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.empty()) {
        send(user->clientSocket,
            (":" + serverName + " 461 PASS :Not enough parameters\r\n").c_str());
        return;
    }
    if (user->registrationState != REG_STATE_INIT) {
        send(user->clientSocket,
            (":" + serverName + " 462 PASS :You may not reregister\r\n").c_str());
        return;
    }
    if (msg.params[0] == _password) {
        user->registrationState = REG_STATE_PASS;
        send(user->clientSocket,
            (":" + serverName + " NOTICE " +
            (user->nickname.empty() ? "*" : user->nickname) +
            " :Password accepted\r\n").c_str());
    } else {
        send(user->clientSocket,
            (":" + serverName + " 464 PASS :Password incorrect\r\n").c_str());
    }
}

// NICK
void IRC_Protocol::handleNICK(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.empty()) {
        send(user->clientSocket,
            (":" + serverName + " 461 NICK :Not enough parameters\r\n").c_str());
        return;
    }
    if (user->registrationState < REG_STATE_PASS) {
        send(user->clientSocket,
            (":" + serverName + " 451 NICK :You must send PASS before NICK\r\n").c_str());
        return;
    }
    std::string newNick = msg.params[0];
    // Çakışma kontrolü
    std::map<int, IRCUser*>::iterator uit;
    for (uit = connectedUsers.begin(); uit != connectedUsers.end(); ++uit) {
        if (uit->second->nickname == newNick) {
            send(user->clientSocket,
                (":" + serverName + " 433 * " + newNick + " :Nickname is already in use\r\n").c_str());
            return;
        }
    }
    user->nickname = newNick;
    user->registrationState = REG_STATE_NICK;
    send(user->clientSocket,
        (":" + serverName + " NICK " + newNick + "\r\n").c_str());
}

// USER
void IRC_Protocol::handleUSER(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.size() < 4) {
        send(user->clientSocket,
            (":" + serverName + " 461 USER :Not enough parameters\r\n").c_str());
        return;
    }
    if (user->registrationState < REG_STATE_NICK) {
        send(user->clientSocket,
            (":" + serverName + " 451 USER :You must send NICK before USER\r\n").c_str());
        return;
    }
    user->username = msg.params[0];
    user->realname = msg.params[3];
    user->registrationState = REG_STATE_USER;
    send(user->clientSocket,
        (":" + serverName + " 001 " + user->nickname +
         " :Welcome to the IRC Network, " + user->nickname + "!\r\n").c_str());
}

// PRIVMSG
void IRC_Protocol::handlePRIVMSG(const IRCMessage& msg, IRCUser* user) {
    // 1) Parametre sayısı
    if (msg.params.size() < 2) {
        send(user->clientSocket,
            (":" + serverName + " 461 PRIVMSG :Not enough parameters\r\n").c_str());
        return;
    }

    std::string target = msg.params[0];
    std::string text   = msg.params[1];
    std::string prefix = ":" + user->nickname + "!" + user->username + "@" + serverName;
    std::string resp   = prefix + " PRIVMSG " + target + " :" + text + "\r\n";

    // 2) Kanal mesajı mı?
    if (!target.empty() && target[0] == '#') {
        // Kanal var mı?
        std::map<std::string, IRCChannel*>::iterator cit = channels.find(target);
        if (cit == channels.end()) {
            send(user->clientSocket,
                (":" + serverName + " 403 " + target + " :No such channel\r\n").c_str());
            return;
        }
        IRCChannel* ch = cit->second;

        // Gönderen kanalda mı?
        bool inChannel = false;
        for (std::vector<IRCUser*>::iterator uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
            if (*uit == user) { inChannel = true; break; }
        }
        if (!inChannel) {
            send(user->clientSocket,
                (":" + serverName + " 442 " + user->nickname + " " + target +
                 " :You're not on that channel\r\n").c_str());
            return;
        }

        // 3) Özel bot komutu (!joke)
        if (text == "!joke") {
            std::vector<std::string> jokes;
            jokes.push_back("Neden bilgisayar çöker? Çünkü pencereyi açmıştır.");
            jokes.push_back("Programcı sabah kahve içmeden boot edemez.");
            jokes.push_back("Segmentation Fault: Beni unuttun mu?");
            jokes.push_back("Kod yazdım, çalıştı. Nedenini bilmiyorum.");
            jokes.push_back("while(!dead) { code(); }");
            jokes.push_back("Bilgisayarcı denize düşmüş. 'CTRL'ü kaybettim!' diye bağırmış.");
            jokes.push_back("Neden programcılar doğaya çıkmaz? Çünkü 'runtime environment' yok.");
            jokes.push_back("Segmentation fault nedir? Hafızanızı kaybettiğiniz an.");
            jokes.push_back("Kod: çalışmaz. Kod: düzeltirsin. Kod: yine çalışmaz. Kod: geri alırsın. Kod: ÇALIŞIR.");
            jokes.push_back("Sistem yöneticisi ne zaman güler? root olarak login olunca.");
            jokes.push_back("Neden C programcıları 'Halloween'de kıyafet giymez? Çünkü onlar zaten pointer.");
            jokes.push_back("while(alive) { code(); } // Sonsuz döngüdeyim...");
            jokes.push_back("Kod yazdım, çalıştı. Paniklemeye başladım.");
            jokes.push_back("Hataları seviyorum. Onlar olmadan debugger ne işe yarar?");
            jokes.push_back("En iyi programcı kimdir? Kodunu sabah unutmayan!");
            jokes.push_back("Neden Java kahve sever? Çünkü o zaten bir kahve çekirdeği.");
            jokes.push_back("Bilgisayarcı düğünde ne der? 'Merge complete!'");
            srand(time(NULL));
            std::string joke = jokes[rand() % jokes.size()];

            // Bot'u bul
            for (std::vector<IRCUser*>::iterator uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
                if ((*uit)->isBot) {
                    std::string jokeMsg = ":" + (*uit)->nickname + "!bot@" + serverName +
                                          " PRIVMSG " + ch->name + " :" + joke + "\r\n";
                    for (std::vector<IRCUser*>::iterator it = ch->users.begin(); it != ch->users.end(); ++it) {
                        send((*it)->clientSocket, jokeMsg.c_str());
                    }
                    return;
                }
            }
        }

        // Normal broadcast (gönderen hariç)
        for (std::vector<IRCUser*>::iterator uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
            if (*uit != user) {
                send((*uit)->clientSocket, resp.c_str());
            }
        }
        return;
    }

    // 4) Bire bir mesaj (PM)
    int targetSock = -1;
    for (std::map<int, IRCUser*>::iterator sit = connectedUsers.begin(); sit != connectedUsers.end(); ++sit) {
        if (sit->second->nickname == target) {
            targetSock = sit->first;
            break;
        }
    }
    if (targetSock == -1) {
        send(user->clientSocket,
            (":" + serverName + " 401 " + target + " :No such nick/channel\r\n").c_str());
        return;
    }

    send(targetSock, resp.c_str());
}


void IRC_Protocol::handleJOIN(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.empty()) {
        send(user->clientSocket,
            (":" + serverName + " 461 JOIN :Not enough parameters\r\n").c_str());
        return;
    }

    std::string chName = msg.params[0];
    std::string key    = (msg.params.size() > 1 ? msg.params[1] : "");

    // Kanal ismi geçerli mi? IRC'de genelde # ile başlar
    if (chName.empty() || chName[0] != '#') {
        send(user->clientSocket,
            (":" + serverName + " 403 " + chName + " :Invalid channel name\r\n").c_str());
        return;
    }

    IRCChannel* ch = NULL;
    std::map<std::string, IRCChannel*>::iterator cit = channels.find(chName);
    bool isNewChannel = (cit == channels.end());

    if (isNewChannel) {
        ch = new IRCChannel(chName, user);
        channels[chName] = ch;
    } else {
        ch = cit->second;

        // +i kontrolü
        if (ch->inviteOnly) {
            bool invited = false;
            for (std::vector<IRCUser*>::iterator iit = ch->invitedUsers.begin(); iit != ch->invitedUsers.end(); ++iit) {
                if (*iit == user) { invited = true; break; }
            }
            if (!invited) {
                send(user->clientSocket,
                    (":" + serverName + " 473 " + user->nickname + " " + chName +
                     " :Cannot join channel (+i)\r\n").c_str());
                return;
            }
        }

        // +k kontrolü
        if (!ch->key.empty() && ch->key != key) {
            send(user->clientSocket,
                (":" + serverName + " 475 " + user->nickname + " " + chName +
                 " :Cannot join channel (+k)\r\n").c_str());
            return;
        }

        // +l kontrolü
        if (ch->limit > 0 && (int)ch->users.size() >= ch->limit) {
            send(user->clientSocket,
                (":" + serverName + " 471 " + user->nickname + " " + chName +
                 " :Cannot join channel (+l)\r\n").c_str());
            return;
        }
    }

    // Kullanıcıyı ekle (henüz eklenmemişse)
    std::vector<IRCUser*>::iterator uit;
    bool already = false;
    for (uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
        if (*uit == user) { already = true; break; }
    }
    if (!already) {
        ch->users.push_back(user);
        user->channels.push_back(chName);

        // BOT varsa yeni gelen kullanıcıya özelden mesaj atsın
        for (std::vector<IRCUser*>::iterator bit = ch->users.begin(); bit != ch->users.end(); ++bit) {
            IRCUser* possibleBot = *bit;
            if (possibleBot->username == "bot") {
                std::string welcomeMsg = ":" + possibleBot->nickname + "!bot@" + serverName +
                                         " PRIVMSG " + user->nickname +
                                         " :Hoş geldin " + user->nickname + ", kanala hoş geldin!\r\n";
                send(user->clientSocket, welcomeMsg.c_str());
                break;
            }
        }
    }

    // JOIN bildirimi tüm üyelere
    std::string joinMsg = ":" + user->nickname + "!" + user->username + "@" +
                          serverName + " JOIN " + chName + "\r\n";
    for (uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
        send((*uit)->clientSocket, joinMsg.c_str());
    }

    // Eğer kanal yeni oluşturulduysa, kullanıcıya +o verildiğini bildir
    if (isNewChannel) {
        std::string modeMsg = ":" + serverName + " MODE " + chName +
                              " +o " + user->nickname + "\r\n";
        send(user->clientSocket, modeMsg.c_str());
    }

    // NAMES yanıtı
    std::string namesList = ":" + serverName + " 353 " + user->nickname +
                            " = " + chName + " :";
    for (uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
        namesList += (*uit)->nickname + " ";
    }
    namesList += "\r\n";
    send(user->clientSocket, namesList.c_str());

    // End of NAMES
    std::string endNames = ":" + serverName + " 366 " + user->nickname +
                           " " + chName + " :End of /NAMES list\r\n";
    send(user->clientSocket, endNames.c_str());
}


// PART
void IRC_Protocol::handlePART(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.empty()) {
        send(user->clientSocket,
            (":" + serverName + " 461 PART :Not enough parameters\r\n").c_str());
        return;
    }

    std::string chName = msg.params[0];
    std::string reason = (msg.params.size() > 1 ? msg.params[1] : "Leaving");

    std::map<std::string, IRCChannel*>::iterator cit = channels.find(chName);
    if (cit == channels.end()) {
        send(user->clientSocket,
            (":" + serverName + " 403 " + chName + " :No such channel\r\n").c_str());
        return;
    }

    IRCChannel* ch = cit->second;

    // Kullanıcı kanalda mı?
    std::vector<IRCUser*>::iterator uit;
    for (uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
        if (*uit == user)
            break;
    }
    if (uit == ch->users.end()) {
        send(user->clientSocket,
            (":" + serverName + " 442 " + user->nickname + " " + chName +
             " :You're not on that channel\r\n").c_str());
        return;
    }

    // PART mesajı
    std::string partMsg = ":" + user->nickname + "!" + user->username + "@" +
                          serverName + " PART " + chName + " :" + reason + "\r\n";

    // Tüm kullanıcılara bildir
    for (std::vector<IRCUser*>::iterator u = ch->users.begin(); u != ch->users.end(); ++u) {
        send((*u)->clientSocket, partMsg.c_str());
    }

    // Listelerden çıkar
    ch->users.erase(uit);
    ch->operators.erase(std::remove(ch->operators.begin(), ch->operators.end(), user), ch->operators.end());

    for (std::vector<std::string>::iterator sit = user->channels.begin(); sit != user->channels.end(); ++sit) {
        if (*sit == chName) {
            user->channels.erase(sit);
            break;
        }
    }

    // Kanal boşsa tamamen sil
    if (ch->users.empty()) {
        channels.erase(chName);
        delete ch;
    } else {
        // Operatör yoksa yenisini ata ve bildir
        if (ch->operators.empty()) {
            IRCUser* newOp = ch->users.front();
            ch->operators.push_back(newOp);

            std::string modeMsg = ":" + serverName + " MODE " + chName +
                                  " +o " + newOp->nickname + "\r\n";
            for (std::vector<IRCUser*>::iterator u = ch->users.begin(); u != ch->users.end(); ++u) {
                send((*u)->clientSocket, modeMsg.c_str());
            }
        }
    }
}

void IRC_Protocol::handleQUIT(const IRCMessage& msg, IRCUser* user) {
    std::string reason = msg.params.empty() ? "Client Quit" : msg.params[0];
    std::string quitMsg = ":" + user->nickname + "!" + user->username + "@" +
                          serverName + " QUIT :" + reason + "\r\n";

    // Tüm kanalları kopyalayarak işle (iterator invalidation'a karşı)
    std::vector<std::string> userChannels = user->channels;

    for (std::vector<std::string>::iterator sit = userChannels.begin(); sit != userChannels.end(); ++sit) {
        std::map<std::string, IRCChannel*>::iterator cit = channels.find(*sit);
        if (cit == channels.end())
            continue; // ekstra güvenlik kontrolü

        IRCChannel* ch = cit->second;

        // Tüm diğer kullanıcılara QUIT mesajı gönder
        for (std::vector<IRCUser*>::iterator uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
            if (*uit != user) {
                send((*uit)->clientSocket, quitMsg.c_str());
            }
        }

        // Kullanıcıyı kanaldan çıkar
        ch->users.erase(std::remove(ch->users.begin(), ch->users.end(), user), ch->users.end());
        ch->operators.erase(std::remove(ch->operators.begin(), ch->operators.end(), user), ch->operators.end());

        // Kanal boşsa sil
        if (ch->users.empty()) {
            channels.erase(*sit);
            delete ch;
        }
        else {
            // Operatör yoksa yeni birini ata ve bildir
            if (ch->operators.empty() && !ch->users.empty()) {
                IRCUser* newOp = ch->users.front();
                ch->operators.push_back(newOp);

                std::string modeMsg = ":" + serverName + " MODE " + ch->name +
                                      " +o " + newOp->nickname + "\r\n";

                for (std::vector<IRCUser*>::iterator uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
                    send((*uit)->clientSocket, modeMsg.c_str());
                }
            }
        }
    }
    
    // Tüm kaynakları temizle
    connectedUsers.erase(user->clientSocket);
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(user->clientSocket == fds[i].fd)
        {
            fds[i].fd = -1;
            fds[i].events = 0;
            fds[i].revents = 0;
            nfds--;
        }
    } 
    delete user;
    
}

// WHO
void IRC_Protocol::handleWHO(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.empty()) {
        send(user->clientSocket,
            (":" + serverName + " 461 WHO :Not enough parameters\r\n").c_str());
        return;
    }
    std::string chName = msg.params[0];
    std::map<std::string, IRCChannel*>::iterator cit = channels.find(chName);
    if (cit == channels.end()) {
        send(user->clientSocket,
            (":" + serverName + " 403 " + chName + " :No such channel\r\n").c_str());
        return;
    }
    IRCChannel* ch = cit->second;
    std::vector<IRCUser*>::iterator uit;
    for (uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
        send(user->clientSocket,
            (":" + serverName + " 352 " + user->nickname + " " + chName +
             " host " + serverName + " " + (*uit)->nickname +
             " H :0 " + (*uit)->realname + "\r\n").c_str());
    }
    send(user->clientSocket,
        (":" + serverName + " 315 " + user->nickname + " " + chName +
         " :End of WHO list\r\n").c_str());
}

// LIST
void IRC_Protocol::handleLIST(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.empty()) {
        std::map<std::string, IRCChannel*>::iterator cit;
        for (cit = channels.begin(); cit != channels.end(); ++cit) {
            // kullanıcı sayısını dönüştür
            std::ostringstream oss;
            oss << cit->second->users.size();
            send(user->clientSocket,
                (":" + serverName + " 322 " + user->nickname + " " +
                 cit->first + " :" + oss.str() + "\r\n").c_str());
        }
        send(user->clientSocket,
            (":" + serverName + " 323 " + user->nickname + " :End of /LIST\r\n").c_str());
    }
    else {
        // Parametreli listeleme var ise buraya ekleyin
    }
}

// TOPIC
void IRC_Protocol::handleTOPIC(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.empty()) {
        send(user->clientSocket,
            (":" + serverName + " 461 TOPIC :Not enough parameters\r\n").c_str());
        return;
    }
    std::string chName = msg.params[0];

    std::map<std::string, IRCChannel*>::iterator cit = channels.find(chName);
    if (cit == channels.end()) {
        send(user->clientSocket,
            (":" + serverName + " 403 " + chName + " :No such channel\r\n").c_str());
        return;
    }
    IRCChannel* ch = cit->second;

    // *** Burada iteratörü tanımlıyoruz ***
    std::vector<IRCUser*>::iterator uit;

    if (msg.params.size() == 1) {
        // Sadece konu gösterme
        send(user->clientSocket,
            (":" + serverName + " 332 " + user->nickname + " " + chName +
             " :" + ch->topic + "\r\n").c_str());
    }
    else {
        // Değiştirme izni kontrolü (+t modu ve operatörlük)
        bool isOp = false;
        std::vector<IRCUser*>::iterator oit;
        for (oit = ch->operators.begin(); oit != ch->operators.end(); ++oit) {
            if (*oit == user) { isOp = true; break; }
        }
        if (ch->topicOnlyOps && !isOp) {
            send(user->clientSocket,
                (":" + serverName + " 482 " + user->nickname + " " + chName +
                 " :You're not channel operator\r\n").c_str());
            return;
        }

        // Konuyu güncelle
        ch->topic = msg.params[1];
        std::string tmsg = ":" + user->nickname + "!" + user->username + "@" +
                           serverName + " TOPIC " + chName + " :" + ch->topic + "\r\n";

        // Tüm üyelere bildir
        for (uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
            send((*uit)->clientSocket, tmsg.c_str());
        }
    }
}

// MODE
void IRC_Protocol::handleMODE(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.empty()) {
        send(user->clientSocket,
            (":" + serverName + " 461 MODE :Not enough parameters\r\n").c_str());
        return;
    }

    std::string chName = msg.params[0];
    std::map<std::string, IRCChannel*>::iterator cit = channels.find(chName);
    if (cit == channels.end()) {
        send(user->clientSocket,
            (":" + serverName + " 403 " + chName + " :No such channel\r\n").c_str());
        return;
    }

    IRCChannel* ch = cit->second;

    // === 1) Sadece bilgi sorgusuysa (örneğin MODE #channel) ===
    if (msg.params.size() == 1) {
        std::string modes = "+";
        std::string modeArgs;

        if (ch->inviteOnly) modes += "i";
        if (ch->topicOnlyOps) modes += "t";
        if (!ch->key.empty()) {
            modes += "k";
            // Eğer kullanıcı içerideyse key göster, değilse *
            bool inChan = false;
            for (size_t i = 0; i < ch->users.size(); ++i) {
                if (ch->users[i] == user) {
                    inChan = true;
                    break;
                }
            }
            modeArgs += " ";
            modeArgs += (inChan ? ch->key : "*");
        }
        if (ch->limit > 0) {
            modes += "l";
            std::ostringstream oss;
            oss << ch->limit;
            modeArgs += " " + oss.str();
        }

        std::string reply = ":" + serverName + " 324 " + user->nickname + " " + chName +
                            " " + modes + modeArgs + "\r\n";
        send(user->clientSocket, reply.c_str());

        // RFC opsiyonel: 329 numarası ile timestamp
        std::ostringstream ts;
        ts << std::time(NULL);
        std::string tsReply = ":" + serverName + " 329 " + user->nickname + " " + chName + " " + ts.str() + "\r\n";
        send(user->clientSocket, tsReply.c_str());
        return;
    }

    // === 2) MOD ATAMA işlemleri ===
    std::string modes = msg.params[1];
    bool isOp = false;
    for (std::vector<IRCUser*>::iterator oit = ch->operators.begin(); oit != ch->operators.end(); ++oit) {
        if (*oit == user) { isOp = true; break; }
    }
    if (!isOp) {
        send(user->clientSocket,
            (":" + serverName + " 482 " + user->nickname + " " + chName +
             " :You're not channel operator\r\n").c_str());
        return;
    }

    bool add = true;
    for (size_t i = 0; i < modes.size(); ++i) {
        char c = modes[i];
        if (c == '+') add = true;
        else if (c == '-') add = false;
        else {
            if (c == 'i') ch->inviteOnly = add;
            else if (c == 't') ch->topicOnlyOps = add;
            else if (c == 'k') {
                if (add && msg.params.size() > 2) {
                    ch->key = msg.params[2];
                } else if (!add) {
                    ch->key.clear();
                }
            }
            else if (c == 'l') {
                if (add && msg.params.size() > 2) {
                    ch->limit = std::atoi(msg.params[2].c_str());
                } else if (!add) {
                    ch->limit = 0;
                }
            }
            else if (c == 'o' && msg.params.size() > 2) {
                std::string nick = msg.params[2];
                for (size_t j = 0; j < ch->users.size(); ++j) {
                    if (ch->users[j]->nickname == nick) {
                        if (add)
                            ch->operators.push_back(ch->users[j]);
                        else
                            ch->operators.erase(std::remove(ch->operators.begin(), ch->operators.end(), ch->users[j]), ch->operators.end());
                        break;
                    }
                }
            }
        }
    }

    // Bildirimi tüm üyelere yayınla
    std::string reply = ":" + user->nickname + "!" + user->username + "@" +
                        serverName + " MODE " + chName + " " + modes;
    if (msg.params.size() > 2) reply += " " + msg.params[2];
    reply += "\r\n";

    for (size_t i = 0; i < ch->users.size(); ++i) {
        send(ch->users[i]->clientSocket, reply.c_str());
    }
}


// INVITE
void IRC_Protocol::handleINVITE(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.size() < 2) {
        send(user->clientSocket,
            (":" + serverName + " 461 INVITE :Not enough parameters\r\n").c_str());
        return;
    }
    std::string nick   = msg.params[0];
    std::string chName = msg.params[1];
    std::map<std::string, IRCChannel*>::iterator cit = channels.find(chName);
    if (cit == channels.end()) {
        send(user->clientSocket,
            (":" + serverName + " 403 " + chName + " :No such channel\r\n").c_str());
        return;
    }
    IRCChannel* ch = cit->second;
    bool isOp = false;
    std::vector<IRCUser*>::iterator oit2;
    for (oit2 = ch->operators.begin(); oit2 != ch->operators.end(); ++oit2) {
        if (*oit2 == user) { isOp = true; break; }
    }
    if (! isOp) {
        send(user->clientSocket,
            (":" + serverName + " 482 " + user->nickname + " " + chName +
             " :You're not channel operator\r\n").c_str());
        return;
    }
    std::map<int, IRCUser*>::iterator uit4;
    for (uit4 = connectedUsers.begin(); uit4 != connectedUsers.end(); ++uit4) {
        if (uit4->second->nickname == nick) {
            ch->invitedUsers.push_back(uit4->second);
            send(uit4->first,
                (":" + user->nickname + "!" + user->username + "@" + serverName +
                 " INVITE " + nick + " " + chName + "\r\n").c_str());
            send(user->clientSocket,
                (":" + serverName + " NOTICE " + user->nickname +
                 " :Invitation sent to " + nick + "\r\n").c_str());
            return;
        }
    }
    send(user->clientSocket,
        (":" + serverName + " 401 " + nick + " :No such nick\r\n").c_str());
}

// BOT
void IRC_Protocol::handleBOT(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.size() < 2) {
        send(user->clientSocket,
            (":" + serverName + " 461 BOT :Not enough parameters\r\n").c_str());
        return;
    }

    std::string botNick = msg.params[0];
    std::string chName = msg.params[1];

    std::map<std::string, IRCChannel*>::iterator cit = channels.find(chName);
    if (cit == channels.end()) {
        send(user->clientSocket,
            (":" + serverName + " 403 " + chName + " :No such channel\r\n").c_str());
        return;
    }

    IRCChannel* ch = cit->second;

    // Operatör kontrolü
    bool isOp = false;
    for (size_t i = 0; i < ch->operators.size(); ++i) {
        if (ch->operators[i] == user) {
            isOp = true;
            break;
        }
    }
    if (!isOp) {
        send(user->clientSocket,
            (":" + serverName + " 482 " + user->nickname + " " + chName +
             " :You're not channel operator\r\n").c_str());
        return;
    }

    // Bot user oluştur
    int sock = -100 - static_cast<int>(connectedUsers.size());  // negatif fd
    IRCUser* bot = new IRCUser(sock);
    bot->nickname = botNick;
    bot->username = "bot";
    bot->realname = "IRC Bot";
    bot->registrationState = REG_STATE_USER;
    bot->isBot = true;

    connectedUsers[sock] = bot;
    ch->users.push_back(bot);
    bot->channels.push_back(chName);

    // JOIN + Greet
    std::string joinMsg = ":" + botNick + "!bot@" + serverName +
                          " JOIN " + chName + "\r\n";
    std::string greetMsg = ":" + botNick + "!bot@" + serverName +
                           " PRIVMSG " + chName + " :Hello, I am a bot!\r\n";

    for (size_t i = 0; i < ch->users.size(); ++i) {
        send(ch->users[i]->clientSocket, joinMsg.c_str());
        send(ch->users[i]->clientSocket, greetMsg.c_str());
    }

    send(user->clientSocket,
        (":" + serverName + " NOTICE " + user->nickname +
         " :Bot " + botNick + " added to " + chName + "\r\n").c_str());
}

// CAP
void IRC_Protocol::handleCAP(const IRCMessage& msg, IRCUser* user) {
    if (msg.params.empty()) {
        send(user->clientSocket,
            (":" + serverName + " 461 CAP :Not enough parameters\r\n").c_str());
        return;
    }
    if (msg.params[0] == "LS") {
        send(user->clientSocket,
            (":" + serverName + " CAP * :\r\n").c_str());
    } else {
        send(user->clientSocket,
            (":" + serverName + " 410 CAP :Unsupported subcommand\r\n").c_str());
    }
}

// PING
void IRC_Protocol::handlePING(const IRCMessage& msg, IRCUser* user) {
    std::string payload = (msg.params.empty() ? serverName : msg.params[0]);
    std::string response = "PONG :" + payload + "\r\n";
    send(user->clientSocket, response.c_str());
}

// KICK
void IRC_Protocol::handleKICK(const IRCMessage& msg, IRCUser* user) {
    // 1) Parametre sayısı
    if (msg.params.size() < 2) {
        send(user->clientSocket,
            (":" + serverName + " 461 KICK :Not enough parameters\r\n").c_str());
        return;
    }
    const std::string& channelName = msg.params[0];
    const std::string& targetNick  = msg.params[1];

    // 2) Kanal var mı?
    std::map<std::string, IRCChannel*>::iterator cit = channels.find(channelName);
    if (cit == channels.end()) {
        send(user->clientSocket,
            (":" + serverName + " 403 " + channelName + " :No such channel\r\n").c_str());
        return;
    }
    IRCChannel* ch = cit->second;

    // 3) Atma yetkisi (operator) kontrolü
    bool isOp = false;
    for (std::vector<IRCUser*>::iterator oit = ch->operators.begin(); oit != ch->operators.end(); ++oit) {
        if (*oit == user) { isOp = true; break; }
    }
    if (!isOp) {
        send(user->clientSocket,
            (":" + serverName + " 482 " + user->nickname + " " + channelName +
             " :You're not channel operator\r\n").c_str());
        return;
    }

    // 4) Hedef kullanıcıyı bul
    IRCUser* targetUser = NULL;
    for (std::vector<IRCUser*>::iterator uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
        if ((*uit)->nickname == targetNick) {
            targetUser = *uit;
            break;
        }
    }
    if (!targetUser) {
        send(user->clientSocket,
            (":" + serverName + " 441 " + targetNick + " " + channelName +
             " :They aren't on that channel\r\n").c_str());
        return;
    }

    // 5) Sebep metni
    std::string reason = (msg.params.size() > 2 ? msg.params[2] : "No reason given");

    // 6) KICK mesajı (broadcast)
    std::string kickMsg = ":" + user->nickname + "!" + user->username + "@" + serverName +
                          " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    for (std::vector<IRCUser*>::iterator uit = ch->users.begin(); uit != ch->users.end(); ++uit) {
        send((*uit)->clientSocket, kickMsg.c_str());
    }

    // 7) Kullanıcı listesinden çıkar
    ch->users.erase(std::remove(ch->users.begin(), ch->users.end(), targetUser), ch->users.end());

    // 8) Eğer sahibi operatörse, operatör listesinden de çıkar
    ch->operators.erase(std::remove(ch->operators.begin(), ch->operators.end(), targetUser),
                        ch->operators.end());

    // 9) Kullanıcının kanal listesinden çıkar
    for (std::vector<std::string>::iterator sit = targetUser->channels.begin();
         sit != targetUser->channels.end(); ++sit)
    {
        if (*sit == channelName) {
            targetUser->channels.erase(sit);
            break;
        }
    }

    // 10) Kanal boşaldıysa tamamen sil
    if (ch->users.empty()) {
        channels.erase(channelName);
        delete ch;
    }
}
