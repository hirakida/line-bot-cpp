#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_request.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include "line_bot_cppcms.h"

class app : public cppcms::application {
public:
    app(cppcms::service &srv) : cppcms::application(srv) {
        dispatcher().assign("/callback", &app::callback, this);
        mapper().assign("callback", "/callback");
        mapper().root("/");
    }

    void callback();
};

void app::callback() {

    if (!line_bot::is_callback_request(request())) {
        response().make_error_response(404);
    }

    picojson::value val;
    picojson::array &events = line_bot::model::parse_events(val, request());
    for (auto obj : events) {
        picojson::object &event = obj.get<picojson::object>();
        if (line_bot::model::is_message_event(event)) {
            if (line_bot::model::is_text(event)) {
                line_bot::client::reply_text(line_bot::model::replyToken(event),
                                             line_bot::model::text(event));
            } else if (line_bot::model::is_sticker(event)) {
                line_bot::client::reply_sticker(line_bot::model::replyToken(event),
                                                line_bot::model::packageId(event),
                                                line_bot::model::stickerId(event));
            }
            line_bot::client::push_text(line_bot::model::userId(event), "push message");
            line_bot::client::push_sticker(line_bot::model::userId(event), "2", "144");
        }
    }
}

int main(int argc, char **argv) {
    try {
        cppcms::service srv(argc, argv);
        srv.applications_pool()
                .mount(cppcms::applications_factory<app>());
        srv.run();
    }
    catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;
    }
}
