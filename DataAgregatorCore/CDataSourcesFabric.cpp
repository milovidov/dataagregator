/*
Copyright 2016-2019 Mikhail Milovidov <milovidovmikhail@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Precompiled.h"
#include "CDataSourcesFabric.h"

using namespace dataagregatorcore;


namespace {

constexpr const char* g_typeSources = "sources";

constexpr const char* g_typeField = "type";
constexpr const char* g_hostField = "host";
constexpr const char* g_authorizationField = "authorization";
constexpr const char* g_passwordAuthorizationField = "passwordAuthorization";
constexpr const char* g_commandsField = "commands";
constexpr const char* g_reconnectField = "reconnect";

constexpr const char* g_commandField = "command";
constexpr const char* g_outputExtensionField = "outputExtension";
constexpr const char* g_outputExtensionShortField = "extension";
constexpr const char* g_restart = "restart";

}

CDataSourcesFabric::CDataSourcesFabric()
    : data_sources_type_(QMetaEnum::fromType<CDataSourcesFabric::DataSourcesType>())
{

}

DataSources CDataSourcesFabric::getDataSources(const CDataSourcesFabric::DataSourcesType input_type, const QString& input) const
{
    DataSources result;
    switch (input_type) {
    case DataSourcesType::json:
        result = getFromJson(input);
        break;
    case DataSourcesType::yaml:
        result = getFromYaml(input);
        break;
    }
    return result;
}

QStringList CDataSourcesFabric::supportedSourceTypes() const
{
    QStringList result;
    for (int key_index = 0; key_index < data_sources_type_.keyCount(); key_index++) {
        result << data_sources_type_.valueToKey(data_sources_type_.value(key_index));
    }
    return result;
}

QString CDataSourcesFabric::inputTypeName(const CDataSourcesFabric::DataSourcesType input_type) const
{
    const QMetaEnum data_sources_type_ = QMetaEnum::fromType<CDataSourcesFabric::DataSourcesType>();
    return data_sources_type_.valueToKey(input_type);
}

CDataSourcesFabric::DataSourcesType CDataSourcesFabric::sourceTypeValue(const QString& input_type_name) const
{
    const QMetaEnum data_sources_type_ = QMetaEnum::fromType<CDataSourcesFabric::DataSourcesType>();
    return static_cast<CDataSourcesFabric::DataSourcesType>(data_sources_type_.keyToValue(qPrintable(input_type_name.toLower().trimmed())));
}

bool CDataSourcesFabric::isSourceTypeSopported(const QString& input_type) const
{
    return supportedSourceTypes().contains(input_type.toLower().trimmed());
}

DataSources CDataSourcesFabric::getFromJson(const QString& json) const
{
    QJsonParseError json_parse_error;
    QJsonDocument task_document = QJsonDocument::fromJson(json.toUtf8(), &json_parse_error);

    if (json_parse_error.error != QJsonParseError::NoError)
        qFatal("Cann't parse json: %s", qPrintable(json_parse_error.errorString()));

    return convertDataSources(task_document.toVariant().toMap());
}

DataSources CDataSourcesFabric::getFromYaml(const QString& yaml) const
{
    DataSources result;

    try {
        YAML::Node root_node = YAML::LoadFile(yaml.toStdString());
        QVariantMap result;

        YAML::Node servers_node = root_node[g_typeSources];

        if (servers_node.IsMap()) {

        } else {

        }

    } catch (const YAML::Exception& exception) {
        qFatal("Cann't parse yaml: %s", exception.msg.c_str());
    }

    return result;
}


DataSources CDataSourcesFabric::convertDataSources(const QVariantMap& data_sources) const
{
    DataSources result;

    for (const QString& serverName : data_sources.keys()) {
        const QVariantMap& dataSource = data_sources[serverName].toMap();
        ValidateField(!dataSource.isEmpty(), sourceErrorMessage(serverName, "invalidate parameters format"));

        const QString& connectionType = dataSource[g_typeField].toString();
        const QString& host = dataSource[g_hostField].toString();

        QVariantMap connection = dataSource[g_authorizationField].toMap();
        if (connection.isEmpty())
            connection = dataSource[g_passwordAuthorizationField].toMap();

        const QVariantMap& commands = dataSource[g_commandsField].toMap();
        ValidateField(!commands.isEmpty(), sourceErrorMessage(serverName, QString("%1 field is absent").arg(g_commandsField)));

        const bool reconnect = dataSource[g_reconnectField].toBool();

        const std::vector<RemoteCommand>& remoteCommands = getCommands(commands, serverName);

        result.push_back({serverName, connectionType, host, remoteCommands, connection, reconnect});
    }

    return result;
}

QString CDataSourcesFabric::sourceErrorMessage(const QString& serverName, const QString& error) const
{
    return QString("Error for server name %1 - %2").arg(serverName).arg(error);
}

bool CDataSourcesFabric::ValidateField(const bool isOk, const QString& errorMessage) const
{
    if (!isOk)
        throw std::runtime_error(qPrintable(errorMessage));
    return isOk;
}

std::vector<RemoteCommand> CDataSourcesFabric::getCommands(const QVariantMap& commands, const QString& serverName) const
{
    std::vector<RemoteCommand> result;
    for (const QString& commandName : commands.keys()) {
        const QVariantMap& remote_command_parameters = commands[commandName].toMap();
        const QString& command = remote_command_parameters[g_commandField].toString();
        QString output_extension = remote_command_parameters[g_outputExtensionField].toString();
        if (output_extension.isEmpty())
            output_extension = remote_command_parameters[g_outputExtensionShortField].toString();

        ValidateField(!command.isEmpty(),
                      sourceErrorMessage(serverName, QString("%1 field is absent for %2").arg(g_commandField).arg(commandName)));
        ValidateField(!output_extension.isEmpty(),
                      sourceErrorMessage(serverName, QString("%1 field is absent for %2").arg(g_outputExtensionField).arg(commandName)));

        const bool restart = remote_command_parameters.value(g_restart, false).toBool();

        result.push_back({commandName, command, output_extension, restart});
    }
    return result;
}
