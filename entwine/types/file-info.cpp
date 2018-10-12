/******************************************************************************
* Copyright (c) 2016, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#include <entwine/types/file-info.hpp>
#include <entwine/types/files.hpp>

namespace entwine
{

namespace
{
    std::string toString(const FileInfo::Status status)
    {
        switch (status)
        {
            case FileInfo::Status::Outstanding: return "outstanding";
            case FileInfo::Status::Inserted:    return "inserted";
            case FileInfo::Status::Omitted:     return "omitted";
            case FileInfo::Status::Error:       return "error";
            default: throw std::runtime_error("Invalid file info status");
        }
    }

    FileInfo::Status toStatus(const std::string s)
    {
        if (s == "outstanding") return FileInfo::Status::Outstanding;
        if (s == "inserted")    return FileInfo::Status::Inserted;
        if (s == "omitted")     return FileInfo::Status::Omitted;
        if (s == "error")       return FileInfo::Status::Error;
        throw std::runtime_error("Invalid file info status string");
    }
}

FileInfo::FileInfo(const std::string path, const Status status)
    : m_path(path)
    , m_status(status)
{ }

FileInfo::FileInfo(const Json::Value& json)
    : FileInfo(json.isObject() ? json["path"].asString() : json.asString())
{
    if (m_path.empty())
    {
        throw std::runtime_error("Empty path found in file-info");
    }

    if (!json.isObject()) return;

    if (json.isMember("status"))
    {
        m_status = toStatus(json["status"].asString());
    }

    if (json.isMember("bounds"))
    {
        m_bounds = Bounds(json["bounds"]);
        m_boundsEpsilon = m_bounds.growBy(0.005);
    }

    m_points = json["points"].asUInt64();
    m_metadata = json["metadata"];
    m_pointStats = PointStats(
            json["inserts"].asUInt64(),
            json["outOfBounds"].asUInt64());
    m_message = json["message"].asString();

    if (json.isMember("srs")) m_srs = json["srs"];
    if (json.isMember("origin")) m_origin = json["origin"].asUInt64();
}

Json::Value FileInfo::toPrivateJson() const
{
    Json::Value json;
    json["path"] = m_path;
    if (m_points)
    {
        if (m_bounds.exists()) json["bounds"] = m_bounds.toJson();
        json["points"] = (Json::UInt64)m_points;
    }

    if (m_status != Status::Outstanding) json["status"] = toString(m_status);
    if (m_pointStats.inserts())
    {
        json["inserts"] = (Json::UInt64)m_pointStats.inserts();
    }
    if (m_pointStats.outOfBounds())
    {
        json["outOfBounds"] = (Json::UInt64)m_pointStats.outOfBounds();
    }

    if (!m_message.empty()) json["message"] = m_message;

    return json;
}

Json::Value FileInfo::toSourcesJson() const
{
    Json::Value json;
    json["path"] = m_path;
    if (m_bounds.exists()) json["bounds"] = m_bounds.toJson();

    if (!m_metadata.isNull()) json["metadata"] = m_metadata;
    if (m_origin != invalidOrigin) json["origin"] = (Json::UInt64)m_origin;
    if (m_points) json["points"] = (Json::UInt64)m_points;
    if (!m_srs.empty()) json["srs"] = m_srs.toJson();

    return json;
}

void FileInfo::merge(const FileInfo& b)
{
    if (path() != b.path())
    {
        throw std::runtime_error("Invalid paths to merge");
    }

    if (status() == Status::Outstanding && status() != Status::Outstanding)
    {
        status(b.status());
    }

    pointStats().add(b.pointStats());
}

double densityLowerBound(const FileInfoList& files)
{
    double points(0);

    for (const auto& f : files)
    {
        if (const auto b = f.bounds())
        {
            if (b->area() > 0 && f.points())
            {
                points += f.points();
            }
        }
    }

    return points / areaUpperBound(files);
}

double areaUpperBound(const FileInfoList& files)
{
    double area(0);

    for (const auto& f : files)
    {
        if (const auto b = f.bounds())
        {
            if (b->area() > 0) area += b->area();
        }
    }

    return area;
}

} // namespace entwine

