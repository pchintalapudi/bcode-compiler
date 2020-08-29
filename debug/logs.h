#ifndef DEBUG_LOGS
#define DEBUG_LOGS

#include <string>
#include <sstream>

namespace oops_bcode_compiler
{
    namespace debug
    {
        class logging
        {
        public:
            enum class level
            {
                debug,
                info,
                warning,
                error
            };

            struct logbuilder
            {
                logging &out;
                level lvl;
                std::stringstream builder;
                bool flushed = true;

                struct end_t
                {
                };

                static const end_t end;

                void operator<<(end_t)
                {
                    flushed = true;
                    out.log(lvl, builder.str());
                }

                template <typename msg_t>
                logbuilder &operator<<(const msg_t &msg)
                {
                    this->builder << msg;
                    return *this;
                }

                ~logbuilder()
                {
                    if (!this->flushed)
                    {
                        *this << end;
                    }
                }
            };

        private:
            level output_level = level::warning;

        public:
            void set_level(level loglevel)
            {
                this->output_level = loglevel;
            }

            void log(level loglevel, const std::string &msg);
            void log(level loglevel, const char *msg);
#define logfunc(loglevel)                                                      \
    void loglevel(const std::string &msg) { this->log(level::loglevel, msg); } \
    void loglevel(const char *msg) { this->log(level::loglevel, msg); }
            logfunc(debug);
            logfunc(info);
            logfunc(warning);
            logfunc(error);
#undef logfunc

            logbuilder builder(level lvl)
            {
                return {*this, lvl, {}};
            }
        };

        inline logging logger;
    } // namespace debug
} // namespace oops_bcode_compiler
#endif /* DEBUG_LOGS */
