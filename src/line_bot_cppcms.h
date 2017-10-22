#ifndef line_bot_cppcms_h
#define line_bot_cppcms_h

#include <cppcms/application.h>
#include <cppcms/json.h>
#include <curl/curl.h>

namespace line_bot {

    static bool is_json(cppcms::http::request &request) {
        std::string::size_type pos = request.content_type().find("application/json");
        return pos != std::string::npos;
    }

    static bool is_callback_request(cppcms::http::request &request) {
        // TODO: Signature validation
        return request.request_method() == "POST" && is_json(request);
    }

    static cppcms::json::array parse_events(cppcms::http::request &request) {
        std::pair<void *, ssize_t> post_data = request.raw_post_data();
        std::istringstream ss(std::string((char *) post_data.first, (unsigned long) post_data.second));
        cppcms::json::value data;
        data.load(ss, true);
        cppcms::json::array &events = data["events"].array();
        return events;
    }

    namespace event {

//        struct source {
//            std::string type;
//            std::string userId;
//        };
//
//        struct message {
//            std::string id;
//            std::string type;
//        };
//
//        struct event {
//            std::string replyToken;
//            std::string type;
//            std::string timestamp;
//            struct source source;
//            struct message message;
//        };

        enum event_type {
            event_message,
            event_follow,
            event_unfollow,
            event_join,
            event_leave,
            event_postback,
            event_beacon
        };

        static std::string reply_token(cppcms::json::value &event) {
            return event.get<std::string>("replyToken");
        }

        // event type
        static event_type type(cppcms::json::value &event) {
            std::string type = event.get<std::string>("type");
            if (type == "message") {
                return event_message;
            } else if (type == "unfollow") {
                return event_unfollow;
            } else if (type == "join") {
                return event_join;
            } else if (type == "leave") {
                return event_leave;
            } else if (type == "postback") {
                return event_postback;
            } else if (type == "beacon") {
                return event_beacon;
            } else {
                std::cerr << "unknown event_type: " << type << std::endl;
                throw std::runtime_error(type);
            }
        }

        namespace source {
            static std::string user_id(cppcms::json::value &event) {
                // TODO: group or roomかも判定する
                return event.get<std::string>("source.userId");
            }
        }

        namespace message {

            enum message_type {
                message_text,
                message_image,
                message_video,
                message_audio,
                message_file,
                message_location,
                message_sticker
            };

            // type
            static message_type type(cppcms::json::value &event) {
                std::string type = event.get<std::string>("message.type");
                if (type == "text") {
                    return message_text;
                } else if (type == "image") {
                    return message_image;
                } else if (type == "video") {
                    return message_video;
                } else if (type == "audio") {
                    return message_audio;
                } else if (type == "file") {
                    return message_file;
                } else if (type == "location") {
                    return message_location;
                } else if (type == "sticker") {
                    return message_sticker;
                } else {
                    std::cerr << "unknown message_type: " << type << std::endl;
                    throw std::runtime_error(type);
                }
            }

            // text message
            static std::string text(cppcms::json::value &event) {
                return event.get<std::string>("message.text");
            }

            // sticker message
            static std::string package_id(cppcms::json::value &event) {
                return event.get<std::string>("message.packageId");
            }

            static std::string sticker_id(cppcms::json::value &event) {
                return event.get<std::string>("message.stickerId");
            }
        }
    }

    /**
     * client
     */
    class client {
    public:
        static void reply_text(std::string reply_token, std::string text) {
            cppcms::json::value val;
            val["replyToken"] = reply_token;
            val["messages"][0]["type"] = "text";
            val["messages"][0]["text"] = text;
            post(val.save(), send_reply);
        }

        static void reply_sticker(std::string reply_token, std::string packageId, std::string stickerId) {
            cppcms::json::value val;
            val["replyToken"] = reply_token;
            val["messages"][0]["type"] = "sticker";
            val["messages"][0]["packageId"] = packageId;
            val["messages"][0]["stickerId"] = stickerId;
            post(val.save(), send_reply);
        }

        static void push_text(std::string to, std::string text) {
            cppcms::json::value val;
            val["to"] = to;
            val["messages"][0]["type"] = "text";
            val["messages"][0]["text"] = text;
            post(val.save(), send_push);
        }

        static void push_sticker(std::string to, std::string packageId, std::string stickerId) {
            cppcms::json::value val;
            val["to"] = to;
            val["messages"][0]["type"] = "sticker";
            val["messages"][0]["packageId"] = packageId;
            val["messages"][0]["stickerId"] = stickerId;
            post(val.save(), send_push);
        }

        static void get_profile(std::string user_id, std::string &data) {
            std::string url = "https://api.line.me/v2/bot/profile/" + user_id;
            get(url, data);
        }

    private:
        enum send_type {
            send_reply,
            send_push
        };

        static std::string auth_header() {
            std::string access_token = std::getenv("ACCESS_TOKEN");
            return "Authorization: Bearer " + access_token;
        }

        static size_t write_callback(char *ptr, size_t size, size_t nmemb, std::string *userdata) {
            int dataLength = size * nmemb;
            userdata->append(ptr, dataLength);
            return dataLength;
        }

        static void get(std::string url, std::string &data) {
            CURL *curl = curl_easy_init();
            if (curl == NULL) {
                std::cerr << "curl_easy_init() failed" << std::endl;
                return;
            }
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, auth_header().c_str());

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed. " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
        }

        static void post(std::string val, send_type type) {

            CURL *curl = curl_easy_init();
            if (curl == NULL) {
                std::cerr << "curl_easy_init() failed" << std::endl;
                return;
            }
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, auth_header().c_str());

            std::string url = type == send_reply ? "https://api.line.me/v2/bot/message/reply"
                                                 : "https://api.line.me/v2/bot/message/push";
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, val.c_str());

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed. " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
        }
    };
}

#endif //line_bot_cppcms_h
