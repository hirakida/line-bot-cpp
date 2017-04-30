#ifndef LINE_API_CLIENT_H
#define LINE_API_CLIENT_H

#include <cppcms/json.h>
#include <curl/curl.h>

// request
struct request_source {
    std::string type;
    std::string userId;
};

struct request_message {
    std::string id;
    std::string type;
    std::string text;
};

struct request_event {
    std::string replyToken;
    std::string type;
    unsigned long timestamp;
    request_source source;
    request_message message;
};

struct request_body {
    std::vector<request_event> events;
};

// reply
struct reply_message {
    std::string type;
    std::string text;
};

struct reply_body {
    std::string replyToken;
    reply_message messages[1];
};

namespace cppcms {
    namespace json {
        template<>
        struct traits<request_source> {
            static request_source get(cppcms::json::value const &v) {
                if (v.type() != cppcms::json::is_object)
                    throw cppcms::json::bad_value_cast();
                request_source p;
                p.type = v.get<std::string>("type");
                p.userId = v.get<std::string>("userId");
                return p;
            }
        };

        template<>
        struct traits<request_message> {
            static request_message get(cppcms::json::value const &v) {
                if (v.type() != cppcms::json::is_object)
                    throw cppcms::json::bad_value_cast();
                request_message p;
                p.id = v.get<std::string>("id");
                p.type = v.get<std::string>("type");
                p.text = v.get<std::string>("text");
                return p;
            }
        };

        template<>
        struct traits<request_event> {
            static request_event get(cppcms::json::value const &v) {
                if (v.type() != cppcms::json::is_object)
                    throw cppcms::json::bad_value_cast();
                request_event p;
                p.replyToken = v.get<std::string>("replyToken");
                p.type = v.get<std::string>("type");
                p.timestamp = v.get < unsigned long > ("timestamp");
                p.source = v.get<request_source>("source");
                p.message = v.get<request_message>("message");
                return p;
            }
        };

        template<>
        struct traits<request_body> {
            static request_body get(cppcms::json::value const &v) {
                if (v.type() != cppcms::json::is_object)
                    throw cppcms::json::bad_value_cast();
                request_body p;
                p.events = v.get<std::vector<request_event> >("events");
                return p;
            }
        };
    }
}

static bool is_json(const std::string &content_type) {
    std::string::size_type pos = content_type.find("application/json");
    return pos != std::string::npos;
}

class line_bot_client {
public:

    enum send_type {
        reply,
        push
    };

    static std::vector<request_event> load_events(std::pair<void *, size_t> raw_post_data) {
        std::pair<void *, ssize_t> post = raw_post_data;
        std::istringstream ss(std::string((char *) post.first, (unsigned long) post.second));
        cppcms::json::value val;
        val.load(ss, true);
        request_body body = val.get_value<request_body>();
        return body.events;
    }

    static void reply_text(std::string replyToken, std::string text) {
        std::string data =
                "{ \"replyToken\" : \"" + replyToken +
                "\" ,\"messages\" : [{\"type\" : \"text\", \"text\" : \"" + text + "\"}]}";
        send_text(data, reply);
    }

    static void push_text(std::string to, std::string text) {
        std::string data =
                "{ \"to\" : \"" + to +
                "\" ,\"messages\" : [{\"type\" : \"text\", \"text\" : \"" + text + "\"}]}";
        send_text(data, push);
    }

private:

    static void send_text(std::string data, send_type type) {

        CURL *curl = curl_easy_init();
        if (curl == NULL) {
            std::cerr << "curl_easy_init() failed" << std::endl;
            return;
        }
        std::string access_token = std::getenv("ACCESS_TOKEN");
        std::string header = "Authorization: Bearer " + access_token;

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, header.c_str());

        std::string url = type == reply ? "https://api.line.me/v2/bot/message/reply"
                                        : "https://api.line.me/v2/bot/message/push";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed. " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
};

#endif //LINE_API_CLIENT_H
