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

    auto events = line_bot::parse_events(request());
    for (auto event : events) {
        if (line_bot::is_message_event(event.type)) {
            line_bot::client::reply_text(event.replyToken, event.message.text);
            line_bot::client::push_text(event.source.userId, "push message");
            line_bot::client::push_sticker(event.source.userId, "2", "144");
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
