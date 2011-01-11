#ifndef OSMSCOUT_NUMERICINDEX_H
#define OSMSCOUT_NUMERICINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <cmath>
#include <list>
#include <set>

// Index
#include <osmscout/Cache.h>
#include <osmscout/FileScanner.h>
#include <osmscout/TypeConfig.h>

// Index generation
#include <osmscout/FileWriter.h>
#include <osmscout/Import.h>
#include <osmscout/Progress.h>

#include <osmscout/Util.h>

namespace osmscout {

  /**
    Numeric index handles an index over instance of class <T> where the index criteria
    is of type <N>, where <N> has a numeric nature (usually Id).
    */
  template <class N, class T>
  class NumericIndex
  {
  private:
    /**
      an individual index entry.
      */
    struct IndexEntry
    {
      Id         startId;
      FileOffset fileOffset;
    };

    typedef Cache<Id,std::vector<IndexEntry> > PageCache;

    /**
      Returns the size of a individual cache entry
      */
    struct NumericIndexCacheValueSizer : public PageCache::ValueSizer
    {
      unsigned long GetSize(const std::vector<IndexEntry>& value) const
      {
        return sizeof(IndexEntry)*value.size();
      }
    };

  private:
    std::string                    filepart;
    std::string                    filename;
    unsigned long                  cacheSize;
    mutable FileScanner            scanner;
    uint32_t                       levels;
    uint32_t                       levelSize;
    std::vector<IndexEntry>        root;
    mutable std::vector<PageCache> leafs;

  private:
    size_t GetPageIndex(const std::vector<IndexEntry>& index,
                        Id id) const;

  public:
    NumericIndex(const std::string& filename,
                 unsigned long cacheSize);
    virtual ~NumericIndex();

    bool Load(const std::string& path);

    bool GetOffsets(const std::vector<N>& ids, std::vector<FileOffset>& offsets) const;

    void DumpStatistics() const;
  };

  template <class N, class T>
  NumericIndex<N,T>::NumericIndex(const std::string& filename,
                                  unsigned long cacheSize)
   : filepart(filename),
     cacheSize(cacheSize),
     levels(0),
     levelSize(0)
  {
    // no code
  }

  template <class N, class T>
  NumericIndex<N,T>::~NumericIndex()
  {
    // no code
  }

  /**
    Binary search for index page for given id
    */
  template <class N, class T>
  inline size_t NumericIndex<N,T>::GetPageIndex(const std::vector<IndexEntry>& index,
                                                Id id) const
  {
    size_t size=index.size();
    size_t left=0;
    size_t right=index.size()-1;
    size_t mid;

    while (left<=right) {
      mid=(left+right)/2;
      if (index[mid].startId<=id &&
          (mid+1>=size || index[mid+1].startId>id)) {
        return mid;
      }
      else if (index[mid].startId<id) {
        left=mid+1;
      }
      else {
        right=mid-1;
      }
    }

    return size;
  }

  template <class N, class T>
  bool NumericIndex<N,T>::Load(const std::string& path)
  {
    uint32_t    entries;
    FileOffset  lastLevelPageStart;

    std::cout << "Index " << filepart << ":" << std::endl;

    filename=path+"/"+filepart;

    if (!scanner.Open(filename)) {
      std::cerr << "Cannot open file '" << filename << "'" << std::endl;
      return false;
    }

    scanner.ReadNumber(levels);
    scanner.ReadNumber(levelSize);
    scanner.ReadNumber(entries);
    scanner.Read(lastLevelPageStart);

    //std::cout << filepart <<": " << levels << " " << levelSize << " " << entries << " " << lastLevelPageStart << std::endl;

    // Calculate the number of entries in the first level
    size_t levelEntries=entries;

    for (size_t l=1;l<levels; l++) {
      if (levelEntries%levelSize!=0) {
        levelEntries=levelEntries/levelSize+1;
      }
      else {
        levelEntries=levelEntries/levelSize;
      }
    }

    Id         sio=0;
    FileOffset poo=0;

    std::cout << entries << " entries to index, cach size " << cacheSize << ", " << levelEntries << " index entries in first level" << std::endl;

    root.reserve(levelEntries);

    scanner.SetPos(lastLevelPageStart);

    for (size_t i=0; i<levelEntries; i++) {
      IndexEntry entry;
      Id         si;
      FileOffset po;

      scanner.ReadNumber(si);
      scanner.ReadNumber(po);

      sio+=si;
      poo+=po;

      entry.startId=sio;
      entry.fileOffset=poo;

      root.push_back(entry);
    }

    unsigned long currentCacheSize=cacheSize;
    unsigned long currentLevelSize=levelEntries;
    for (size_t i=0; i<levels; i++) {
      unsigned long resultingCacheSize=currentLevelSize;

      if (currentCacheSize==0) {
        resultingCacheSize=1;
      }
      else if (currentLevelSize>currentCacheSize) {
        resultingCacheSize=currentCacheSize;
        currentCacheSize=0;
      }
      else {
        resultingCacheSize=currentLevelSize;
        currentCacheSize-=currentLevelSize;
      }

      std::cout << "Setting cache size for level " << i+1 << " with " << currentLevelSize << " entries to " << resultingCacheSize << std::endl;

      leafs.push_back(PageCache(resultingCacheSize));

      currentLevelSize=currentLevelSize*levelSize;
    }

    return !scanner.HasError() && scanner.Close();
  }

  template <class N, class T>
  bool NumericIndex<N,T>::GetOffsets(const std::vector<N>& ids,
                                     std::vector<FileOffset>& offsets) const
  {
    offsets.reserve(ids.size());
    offsets.clear();

    if (ids.size()==0) {
      return true;
    }

    for (typename std::vector<N>::const_iterator id=ids.begin();
         id!=ids.end();
         ++id) {

      size_t r=GetPageIndex(root,*id);

      if (r>=root.size()) {
        //std::cerr << "Id " << *id << " not found in root index!" << std::endl;
        continue;
      }

      Id         startId=root[r].startId;
      FileOffset offset=root[r].fileOffset;

      for (size_t level=0; level<levels-1; level++) {
        typename PageCache::CacheRef cacheRef;

        if (!leafs[level].GetEntry(startId,cacheRef)) {
          typename PageCache::CacheEntry cacheEntry(startId);

          cacheRef=leafs[level].SetEntry(cacheEntry);

          if (!scanner.IsOpen() &&
              !scanner.Open(filename)) {
            std::cerr << "Cannot open '" << filename << "'!" << std::endl;
            return false;
          }

          scanner.SetPos(offset);

          cacheRef->value.reserve(levelSize);

          IndexEntry entry;

          entry.startId=0;
          entry.fileOffset=0;

          for (size_t j=0; j<levelSize; j++) {
            Id         idOffset;
            FileOffset fileOffset;

            if (scanner.ReadNumber(idOffset) &&
                scanner.ReadNumber(fileOffset)) {
              entry.startId+=idOffset;
              entry.fileOffset+=fileOffset;

              cacheRef->value.push_back(entry);
            }
          }
        }

        size_t i=GetPageIndex(cacheRef->value,*id);

        if (i>=cacheRef->value.size()) {
          //std::cerr << "Id " << *id << " not found in sub page index!" << std::endl;
          continue;
        }

        startId=cacheRef->value[i].startId;
        offset=cacheRef->value[i].fileOffset;
        //std::cout << "id " << *id <<" => " << i << " " << startId << " " << offset << std::endl;
      }

      if (startId==*id) {
        offsets.push_back(offset);
      }
      else {
        //std::cerr << "Id " << *id << " not found in sub index!" << std::endl;
      }
      //std::cout << "=> Id " << *id <<" => " << startId << " " << offset << std::endl;
    }

    return true;
  }

  template <class N,class T>
  void NumericIndex<N,T>::DumpStatistics() const
  {
    size_t memory=0;
    size_t entries=0;

    entries+=root.size();
    memory+=root.size()+sizeof(IndexEntry);

    for (size_t i=0; i<leafs.size(); i++) {
      entries+=leafs[i].GetSize();
      memory+=leafs[i].GetSize()+leafs[i].GetMemory(NumericIndexCacheValueSizer());
    }

    std::cout << "Index " << filepart << ": " << entries << " entries, memory " << memory << std::endl;
  }

  template <class N,class T>
  class NumericIndexGenerator : public ImportModule
  {
  private:
    std::string description;
    std::string datafile;
    std::string indexfile;

  public:
    NumericIndexGenerator(const std::string& description,
                          const std::string& datafile,
                          const std::string& indexfile);
    virtual ~NumericIndexGenerator();

    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };

  template <class N,class T>
  NumericIndexGenerator<N,T>::NumericIndexGenerator(const std::string& description,
                                                    const std::string& datafile,
                                                    const std::string& indexfile)
   : description(description),
     datafile(datafile),
     indexfile(indexfile)
  {
    // no code
  }

  template <class N,class T>
  NumericIndexGenerator<N,T>::~NumericIndexGenerator()
  {
    // no code
  }

  template <class N,class T>
  std::string NumericIndexGenerator<N,T>::GetDescription() const
  {
    return description;
  }

  template <class N,class T>
  bool NumericIndexGenerator<N,T>::Import(const ImportParameter& parameter,
                                          Progress& progress,
                                          const TypeConfig& typeConfig)
  {
    //
    // Writing index file
    //

    progress.SetAction(std::string("Generating '")+indexfile+"'");

    FileScanner             scanner;
    FileWriter              writer;
    uint32_t                dataCount=0;
    std::vector<Id>         startingIds;
    std::vector<FileOffset> pageStarts;
    FileOffset              lastLevelPageStart;

    if (!writer.Open(indexfile)) {
      progress.Error(std::string("Cannot create '")+indexfile+"'");
      return false;
    }

    if (!scanner.Open(datafile)) {
      progress.Error(std::string("Cannot open '")+datafile+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    uint32_t levels=1;
    size_t   tmp;
    uint32_t indexLevelSize=parameter.GetNumericIndexLevelSize();

    tmp=dataCount;
    while (tmp/indexLevelSize>0) {
      tmp=tmp/indexLevelSize;
      levels++;
    }

    indexLevelSize=(uint32_t)ceil(pow(dataCount,1.0/levels));

    progress.Info(std::string("Index for ")+NumberToString(dataCount)+" data elements will be stored in "+NumberToString(levels)+ " levels using index level size of "+NumberToString(indexLevelSize));

    writer.WriteNumber(levels);         // Number of levels
    writer.WriteNumber(indexLevelSize); // Size of index page
    writer.WriteNumber(dataCount);      // Number of nodes

    writer.GetPos(lastLevelPageStart);

    writer.Write((FileOffset)0); // Write the starting position of the last page

    Id         lastId=0;
    FileOffset lastPos=0;

    progress.Info(std::string("Writing level ")+NumberToString(levels)+" ("+NumberToString(dataCount)+" entries)");

    for (uint32_t d=0; d<dataCount; d++) {
      progress.SetProgress(d,dataCount);

      FileOffset pos;

      scanner.GetPos(pos);

      T data;

      if (!data.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(d+1)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (d%indexLevelSize==0) {
        FileOffset pageStart;

        writer.GetPos(pageStart);

        pageStarts.push_back(pageStart);
        startingIds.push_back(data.id);

        writer.WriteNumber(data.id);
        writer.WriteNumber(pos);
      }
      else {
        writer.WriteNumber((data.id-lastId));
        writer.WriteNumber((pos-lastPos));
      }

      lastPos=pos;
      lastId=data.id;
    }

    levels--;

    while (levels>0) {
      std::vector<Id>         si(startingIds);
      std::vector<FileOffset> po(pageStarts);

      startingIds.clear();
      pageStarts.clear();

      progress.Info(std::string("Writing level ")+NumberToString(levels)+" ("+NumberToString(si.size())+" entries)");

      for (size_t i=0; i<si.size(); i++) {
        if (i%indexLevelSize==0) {
          FileOffset pageStart;

          writer.GetPos(pageStart);

          startingIds.push_back(si[i]);
          pageStarts.push_back(pageStart);
        }

        if (i%indexLevelSize==0) {
          writer.WriteNumber(si[i]);
          writer.WriteNumber(po[i]);
        }
        else {
          writer.WriteNumber((si[i]-si[i-1]));
          writer.WriteNumber(po[i]-po[i-1]);
        }
      }

      levels--;
    }

    writer.SetPos(lastLevelPageStart);

    if (indexLevelSize>0) {
      writer.Write(pageStarts[0]);
    }

    return !scanner.HasError() &&
           scanner.Close() &&
           !writer.HasError() &&
           writer.Close();
  }
}

#endif
