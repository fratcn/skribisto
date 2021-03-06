#include "skrsearchpaperlistproxymodel.h"
#include "plmmodels.h"
#include <QTimer>

SKRSearchPaperListProxyModel::SKRSearchPaperListProxyModel(SKR::ItemType paperType)
    :
      QSortFilterProxyModel(), m_paperType(paperType),
      m_showTrashedFilter(true), m_showNotTrashedFilter(true), m_navigateByBranchesEnabled(
                                                                   false), m_textFilter(""),
      m_projectIdFilter(-2), m_parentIdFilter(-2), m_showParentWhenParentIdFilter(false)
{
    if (paperType == SKR::Sheet) {
        m_paperHub = plmdata->sheetHub();
        m_propertyHub = plmdata->sheetPropertyHub();
        this->setSourceModel(plmmodels->sheetListModel());
    }
    else if (paperType == SKR::Note) {
        m_paperHub = plmdata->noteHub();
        m_propertyHub = plmdata->notePropertyHub();
        this->setSourceModel(plmmodels->noteListModel());
    }
    else {
        qFatal(this->metaObject()->className(), "Invalid ItemType");
    }


    this->setSortRole(SKRPaperItem::SortOrderRole);
    this->setDynamicSortFilter(false);


    connect(plmdata->projectHub(),
            &PLMProjectHub::projectLoaded,
            this,
            &SKRSearchPaperListProxyModel::loadProjectSettings);
    connect(plmdata->projectHub(),
            &PLMProjectHub::projectToBeClosed,
            this,
            &SKRSearchPaperListProxyModel::saveProjectSettings,
            Qt::DirectConnection);
    connect(plmdata->projectHub(), &PLMProjectHub::projectClosed, this,
            [this](int
            projectId) {
        this->clearHistory(projectId);
    });
    connect(plmdata->projectHub(), &PLMProjectHub::projectClosed, this, [this]() {
        this->invalidateFilter();
    });


    // connect this proxy model to all other proxy models using the main model

    SKRPaperListModel *listModel = static_cast<SKRPaperListModel*>(this->sourceModel());
    connect(this, &SKRSearchPaperListProxyModel::sortOtherProxyModelsCalled, listModel, &SKRPaperListModel::sortOtherProxyModelsCalled);
    connect(listModel, &SKRPaperListModel::sortOtherProxyModelsCalled, [this](){
        if(this->sender() != this){
            this->sort(0);
        }
    });
}

Qt::ItemFlags SKRSearchPaperListProxyModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QSortFilterProxyModel::flags(index);

    if (!index.isValid()) return defaultFlags;

    return defaultFlags | Qt::ItemFlag::ItemIsEditable;
}

// -----------------------------------------------------------------------


QVariant SKRSearchPaperListProxyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return QVariant();

    QModelIndex sourceIndex = this->mapToSource(index);
    int col                 = index.column();
    int row                 = index.row();


    SKRPaperItem *item = static_cast<SKRPaperItem *>(sourceIndex.internalPointer());
    int projectId      = item->projectId();
    int paperId        = item->paperId();


    if ((role == Qt::EditRole) && (col == 0)) {
        return this->sourceModel()->data(sourceIndex,
                                         SKRPaperItem::Roles::NameRole).toString();
    }

    if ((role == Qt::CheckStateRole) && (col == 0)) {
        return m_checkedIdsHash.value(paperId, Qt::Unchecked);
    }

    if ((role == SKRPaperItem::Roles::HasChildrenRole) && (col == 0)) {
        return hasChildren(projectId, paperId);
    }

    return QSortFilterProxyModel::data(index, role);
}

// -----------------------------------------------------------------------

bool SKRSearchPaperListProxyModel::setData(const QModelIndex& index,
                                           const QVariant   & value,
                                           int                role)
{
    QModelIndex sourceIndex = this->mapToSource(index);


    SKRPaperItem *item =
            static_cast<SKRPaperItem *>(sourceIndex.internalPointer());

    if ((role == Qt::EditRole) && (sourceIndex.column() == 0)) {
        if (item->isProjectItem()) {
            return this->sourceModel()->setData(sourceIndex,
                                                value,
                                                SKRPaperItem::Roles::ProjectNameRole);
        } else {
            return this->sourceModel()->setData(sourceIndex,
                                                value,
                                                SKRPaperItem::Roles::NameRole);
        }
    }


    if ((role == Qt::CheckStateRole) && (sourceIndex.column() == 0)) {
        int paperId               = item->paperId();
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());
        m_checkedIdsHash.insert(paperId, checkState);

        if ((checkState == Qt::Checked) || (checkState == Qt::Unchecked)) {
            this->checkStateOfAllChildren(item->projectId(), item->paperId(), checkState);
        }
        this->determineCheckStateOfAllAncestors(item->projectId(),
                                                item->paperId(),
                                                checkState);
    }

    return QSortFilterProxyModel::setData(index, value, role);
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::checkStateOfAllChildren(int            projectId,
                                                           int            paperId,
                                                           Qt::CheckState checkState) {
    SKRPaperListModel *model = static_cast<SKRPaperListModel *>(this->sourceModel());

    QList<int> childrenIdsList = this->getChildrenList(projectId,
                                                       paperId,
                                                       m_showTrashedFilter,
                                                       m_showNotTrashedFilter);

    for (int childId : childrenIdsList) {
        m_checkedIdsHash.insert(childId, checkState);
        QModelIndex modelIndex = model->getModelIndex(projectId, childId).first();

        emit dataChanged(this->mapFromSource(modelIndex), this->mapFromSource(modelIndex),
                         QVector<int>() << Qt::CheckStateRole);
    }
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::determineCheckStateOfAllAncestors(
        int            projectId,
        int            paperId,
        Qt::CheckState checkState)
{
    SKRPaperListModel *model    = static_cast<SKRPaperListModel *>(this->sourceModel());
    QList<int> ancestorsIdsList = this->getAncestorsList(projectId,
                                                         paperId,
                                                         m_showTrashedFilter,
                                                         m_showNotTrashedFilter);

    if (ancestorsIdsList.isEmpty()) {
        return;
    }

    // default state :
    Qt::CheckState ancestorCheckState = Qt::Unchecked;

    if (checkState == Qt::Unchecked) {
        // see if the direct ancestor has all its children unchecked
        QList<int> siblingsIdsList = this->getSiblingsList(projectId,
                                                           paperId,
                                                           m_showTrashedFilter,
                                                           m_showNotTrashedFilter);

        if (siblingsIdsList.isEmpty()) {
            ancestorCheckState = Qt::PartiallyChecked;
        }
        else {
            bool areNoneOfTheSiblingsChecked          = false;
            bool areAtLeastOneSiblingChecked          = false;
            bool areAtLeastOneSiblingPartiallyChecked = false;
            bool areAtLeastOneSiblingUnchecked        = false;

            for (int siblingId : siblingsIdsList) {
                Qt::CheckState state = m_checkedIdsHash.value(siblingId, Qt::Unchecked);

                if (state == Qt::Checked) {
                    areAtLeastOneSiblingChecked = true;
                }

                if (state == Qt::PartiallyChecked) {
                    areAtLeastOneSiblingPartiallyChecked = true;
                }

                if (state == Qt::Unchecked) {
                    areAtLeastOneSiblingUnchecked = true;
                }
            }
            areNoneOfTheSiblingsChecked = !areAtLeastOneSiblingChecked &&
                    !areAtLeastOneSiblingPartiallyChecked;

            if (areAtLeastOneSiblingChecked) { // but this one
                ancestorCheckState = Qt::PartiallyChecked;
            }

            if (areAtLeastOneSiblingPartiallyChecked) {
                ancestorCheckState = Qt::PartiallyChecked;
            }
            else if (areNoneOfTheSiblingsChecked) {
                ancestorCheckState = Qt::PartiallyChecked;
            }
        }
    }
    else if (checkState == Qt::Checked) {
        // see if the direct ancestor has all its children checked


        QList<int> siblingsIdsList = this->getSiblingsList(projectId,
                                                           paperId,
                                                           m_showTrashedFilter,
                                                           m_showNotTrashedFilter);

        if (siblingsIdsList.isEmpty()) {
            ancestorCheckState = Qt::Checked;
        }
        else {
            bool areAllSiblingsChecked                = false;
            bool areAtLeastOneSiblingChecked          = false;
            bool areAtLeastOneSiblingPartiallyChecked = false;
            bool areAtLeastOneSiblingUnchecked        = false;

            for (int siblingId : siblingsIdsList) {
                Qt::CheckState state = m_checkedIdsHash.value(siblingId, Qt::Unchecked);

                if (state == Qt::Checked) {
                    areAtLeastOneSiblingChecked = true;
                }

                if (state == Qt::PartiallyChecked) {
                    areAtLeastOneSiblingPartiallyChecked = true;
                }

                if (state == Qt::Unchecked) {
                    areAtLeastOneSiblingUnchecked = true;
                }
            }
            areAllSiblingsChecked = !areAtLeastOneSiblingUnchecked &&
                    !areAtLeastOneSiblingPartiallyChecked;

            if (areAllSiblingsChecked) {
                ancestorCheckState = Qt::Checked;
            }
            else if (areAtLeastOneSiblingUnchecked) { // but this one
                ancestorCheckState = Qt::PartiallyChecked;
            }
            else if (areAtLeastOneSiblingPartiallyChecked) {
                ancestorCheckState = Qt::PartiallyChecked;
            }
        }
    }
    else if (checkState == Qt::PartiallyChecked) {
        ancestorCheckState = Qt::PartiallyChecked;
    }

    if (!m_paperIdListFilter.isEmpty()) {
        QList<int> newList;

        for (int id : ancestorsIdsList) {
            if (m_paperIdListFilter.contains(id)) {
                newList.append(id);
            }
        }

        ancestorsIdsList = newList;
    }

    if (ancestorsIdsList.empty()) {
        return;
    }

    // for (int ancestorId : ancestorsIdsList) {
    m_checkedIdsHash.insert(ancestorsIdsList.first(), ancestorCheckState);
    QModelIndex modelIndex =
            model->getModelIndex(projectId, ancestorsIdsList.first()).first();

    emit dataChanged(this->mapFromSource(modelIndex), this->mapFromSource(modelIndex),
                     QVector<int>() << Qt::CheckStateRole);

    // }
    determineCheckStateOfAllAncestors(m_projectIdFilter,
                                      ancestorsIdsList.first(),
                                      ancestorCheckState);
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setTagIdListFilter(const QList<int>& tagIdListFilter)
{
    m_tagIdListFilter = tagIdListFilter;


    emit tagIdListFilterChanged(tagIdListFilter);

    this->invalidateFilter();
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setHideThoseWithAttributesFilter(const QStringList &hideThoseWithAttributesFilter)
{
    m_hideThoseWithAttributesFilter = hideThoseWithAttributesFilter;

    emit hideThoseWithAttributesFilterChanged(hideThoseWithAttributesFilter);
    this->invalidateFilter();
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setShowOnlyWithAttributesFilter(const QStringList &showOnlyWithAttributesFilter)
{
    m_showOnlyWithAttributesFilter = showOnlyWithAttributesFilter;

    emit showOnlyWithAttributesFilterChanged(showOnlyWithAttributesFilter);
    this->invalidateFilter();
}
// --------------------------------------------------------------

QList<int>SKRSearchPaperListProxyModel::getCheckedIdsList() {
    QList<int> list;

    QHash<int, Qt::CheckState>::const_iterator i = m_checkedIdsHash.constBegin();

    while (i != m_checkedIdsHash.constEnd()) {
        if ((i.value() == Qt::Checked) || (i.value() == Qt::PartiallyChecked)) {
            list << i.key();
        }
        ++i;
    }


    // sort list

    QList<int> allSortedIds = m_paperHub->getAllIds(m_projectIdFilter);
    QList<int> sortedCheckedIds;
    for(int sortedId : allSortedIds){
        if(list.contains(sortedId)){
            sortedCheckedIds.append(sortedId);
        }
    }


    return sortedCheckedIds;
}

// --------------------------------------------------------------


void SKRSearchPaperListProxyModel::setCheckedIdsList(const QList<int>checkedIdsList) {
    SKRPaperListModel *model = static_cast<SKRPaperListModel *>(this->sourceModel());


    this->clearCheckedList();


    if (checkedIdsList.isEmpty()) {
        return;
    }

    for (int i = checkedIdsList.count() - 1; i >= 0; i--) {
        int paperId = checkedIdsList[i];


        if (!this->hasChildren(m_projectIdFilter, paperId)) {
            m_checkedIdsHash.insert(paperId, Qt::Checked);
        }
        else { // has children so verify if there is one checked or partially
            // checked
            Qt::CheckState finalState  = Qt::Unchecked;
            QList<int> childrenIdsList = this->getChildrenList(m_projectIdFilter,
                                                               paperId,
                                                               m_showTrashedFilter,
                                                               m_showNotTrashedFilter);


            bool areAllChildrenChecked              = false;
            bool areAtLeastOneChildChecked          = false;
            bool areAtLeastOneChildPartiallyChecked = false;
            bool areAtLeastOneChildUnchecked        = false;

            for (int childId : childrenIdsList) {
                Qt::CheckState state = m_checkedIdsHash.value(childId, Qt::Unchecked);

                if (state == Qt::Checked) {
                    areAtLeastOneChildChecked = true;
                }
                else if (state == Qt::PartiallyChecked) {
                    areAtLeastOneChildPartiallyChecked = true;
                }
                else {
                    areAtLeastOneChildUnchecked = true;
                }
            }

            areAllChildrenChecked = !areAtLeastOneChildUnchecked &&
                    !areAtLeastOneChildPartiallyChecked;

            if (areAtLeastOneChildUnchecked) { // but this one
                finalState = Qt::PartiallyChecked;
            }
            else if (areAllChildrenChecked) {
                finalState = Qt::Checked;
            }
            else {
                finalState = Qt::PartiallyChecked;
            }


            m_checkedIdsHash.insert(paperId, finalState);
        }

        // m_checkedIdsHash.insert(paperId, Qt::Checked);
        QModelIndexList modelIndexList = model->getModelIndex(m_projectIdFilter, paperId);

        if (modelIndexList.isEmpty()) {
            continue;
        }
        QModelIndex modelIndex = modelIndexList.first();

        emit dataChanged(this->mapFromSource(modelIndex), this->mapFromSource(modelIndex),
                         QVector<int>() << Qt::CheckStateRole);
    }
}

// --------------------------------------------------------------

///
/// \brief SKRSearchPaperListProxyModel::findIdsTrashedAtTheSameTimeThan
/// \param projectId
/// \param paperId
/// \return
/// more or less 1 second between items' trashed dates
QList<int>SKRSearchPaperListProxyModel::findIdsTrashedAtTheSameTimeThan(int projectId,
                                                                        int paperId) {
    QList<int> list;

    QDateTime parentDate = m_paperHub->getTrashedDate(projectId, paperId);


    if (!m_paperIdListFilter.isEmpty()) {
        for (int id : m_paperIdListFilter) {
            QDateTime childDate = m_paperHub->getTrashedDate(projectId, id);

            if (qAbs(childDate.secsTo(parentDate)) < 1) {
                list.append(id);
            }
        }
    }
    else {
        QList<int> childrenIds = this->getChildrenList(projectId,
                                                       paperId,
                                                       m_showTrashedFilter,
                                                       m_showNotTrashedFilter);

        for (int id : childrenIds) {
            QDateTime childDate = m_paperHub->getTrashedDate(projectId, id);

            if (qAbs(childDate.secsTo(parentDate)) < 1) {
                list.append(id);
            }
        }
    }


    return list;
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::deleteDefinitively(int projectId, int paperId)
{
    m_paperHub->removePaper(projectId, paperId);
    sort(0);
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setParentIdFilter(int projectIdfilter)
{
    m_parentIdFilter = projectIdfilter;
    emit parentIdFilterChanged(m_parentIdFilter);

    this->invalidateFilter();
}

// --------------------------------------------------------------


void SKRSearchPaperListProxyModel::setShowParentWhenParentIdFilter(bool showParent)
{
    m_showParentWhenParentIdFilter = showParent;
    emit showParentWhenParentIdFilterChanged(showParent);

    if (m_parentIdFilter != -2) {
        this->invalidateFilter();
    }
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setNavigateByBranchesEnabled(bool navigateByBranches)
{
    m_navigateByBranchesEnabled = navigateByBranches;

    emit navigateByBranchesEnabledChanged(navigateByBranches);
}

// --------------------------------------------------------------


bool SKRSearchPaperListProxyModel::filterAcceptsRow(int                sourceRow,
                                                    const QModelIndex& sourceParent) const
{
    bool value = true;

    QModelIndex index = this->sourceModel()->index(sourceRow, 0, sourceParent);

    if (!index.isValid()) {
        return false;
    }
    SKRPaperItem *item       = static_cast<SKRPaperItem *>(index.internalPointer());
    SKRPaperListModel *model = static_cast<SKRPaperListModel *>(this->sourceModel());

    //        // avoid project item
    //        if (item->data(SKRPaperItem::Roles::PaperIdRole).toInt() == -1) {
    //            value = false;
    //        }

    // displays or not project list :
    if (m_navigateByBranchesEnabled) {
        if ((m_parentIdFilter == -2) && item->isProjectItem()) {
            return true;
        }
        else if ((m_parentIdFilter != -2) && item->isProjectItem()) {
            return false;
        }
        else if ((m_parentIdFilter == -2) && !item->isProjectItem()) {
            return false;
        }
    }
    else if ((m_parentIdFilter == -2) && item->isProjectItem()) {
        return false;
    }

    // project filtering :
    if (value &&
            (item->data(SKRPaperItem::Roles::ProjectIdRole).toInt() == m_projectIdFilter)) {
        value = true;
    }
    else if (value) {
        value = false;
    }

    // trashed filtering :
    if (value && (item->data(SKRPaperItem::Roles::TrashedRole).toBool() == true)) {
        value = m_showTrashedFilter;
    }

    // 'not trashed' filtering :
    if (value && (item->data(SKRPaperItem::Roles::TrashedRole).toBool() == false)) {
        value = m_showNotTrashedFilter;
    }


    if (value &&
            item->data(SKRPaperItem::Roles::NameRole).toString().contains(m_textFilter,
                                                                          Qt::CaseInsensitive))
    {
        value = true;
    }
    else if (value) {
        value = false;
    }

    int paperId = item->data(SKRPaperItem::Roles::PaperIdRole).toInt();

    // paperIdListFiltering :
    if (value && (!m_paperIdListFilter.isEmpty() || !m_hidePaperIdListFilter.isEmpty())) {


        int showed = false;
        if(m_paperIdListFilter.isEmpty()){
            showed = true;
        }
        else if(m_hidePaperIdListFilter.isEmpty()){
            showed = false;
        }
        else { //both are with values
            showed = false;
        }


        if(m_paperIdListFilter.contains(paperId)){
            showed = true;
        }
        if(m_hidePaperIdListFilter.contains(paperId)){
            showed = false;
        }
        value = showed;

    }


    // parentId filtering :
    if (value && (m_parentIdFilter != -2)) {
        if (m_showParentWhenParentIdFilter &&
                (m_parentIdFilter == paperId)) {
            value = true;
        }
        else {
            SKRPaperListModel *model =
                    static_cast<SKRPaperListModel *>(this->sourceModel());
            SKRPaperItem *parentItem = model->getParentPaperItem(item);

            if (parentItem) {
                if (parentItem->paperId() == m_parentIdFilter) {
                    value = true;
                }
                else {
                    value = false;
                }
            }
        }
    }

    // tagId filtering

    if (value && !m_tagIdListFilter.isEmpty() && (m_projectIdFilter != -2)) {
        QList<int> tagIds = plmdata->tagHub()->getTagsFromItemId(m_projectIdFilter,
                                                                 SKR::Note, paperId);

        value = false;

        int tagCountGoal = m_tagIdListFilter.count();
        int tagCount     = 0;

        for (int tag : qAsConst(tagIds)) {
            if (m_tagIdListFilter.contains(tag)) {
                tagCount += 1;
            }
        }

        if (tagCount == tagCountGoal) {
            value = true;
        }
    }

    //  attribute filtering
    if (value && (!m_showOnlyWithAttributesFilter.isEmpty() || !m_hideThoseWithAttributesFilter.isEmpty()) && (m_projectIdFilter != -2)) {
        QStringList attributes = m_propertyHub->getProperty(m_projectIdFilter, paperId, "attributes").split(";", Qt::SkipEmptyParts);

        int showed = false;
        if(m_showOnlyWithAttributesFilter.isEmpty()){
            showed = true;
        }
        else if(m_hideThoseWithAttributesFilter.isEmpty()){
            showed = false;
        }
        else { //both are with values
            showed = false;
        }

        for(const QString &attribute : qAsConst(attributes)){
            if(m_showOnlyWithAttributesFilter.contains(attribute)){
                showed = true;
                break;
            }
            if(m_hideThoseWithAttributesFilter.contains(attribute)){
                showed = false;
                break;
            }

        }


        value = showed;
    }



    return value;
}

void SKRSearchPaperListProxyModel::setProjectIdFilter(int projectIdFilter)
{
    m_projectIdFilter = projectIdFilter;
    emit projectIdFilterChanged(m_projectIdFilter);

    m_parentIdFilter = -2;
    emit parentIdFilterChanged(m_parentIdFilter);

    this->invalidateFilter();
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::clearFilters()
{
    m_projectIdFilter = -2;
    emit projectIdFilterChanged(m_projectIdFilter);

    m_showTrashedFilter = true;
    emit showTrashedFilterChanged(m_showTrashedFilter);

    m_showNotTrashedFilter = true;
    emit showNotTrashedFilterChanged(m_showNotTrashedFilter);

    m_textFilter = "";
    emit textFilterChanged(m_textFilter);

    m_parentIdFilter = -2;
    emit parentIdFilterChanged(m_parentIdFilter);

    m_showParentWhenParentIdFilter = false;
    emit showParentWhenParentIdFilterChanged(m_showParentWhenParentIdFilter);

    this->invalidateFilter();
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::clearCheckedList()
{
    m_checkedIdsHash.clear();
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::checkAll()
{
    SKRPaperListModel *model = static_cast<SKRPaperListModel *>(this->sourceModel());

    if (m_paperIdListFilter.isEmpty()) {
        QList<int> filteredIdsList;
        QList<int> allIdsList = m_paperHub->getAllIds(m_projectIdFilter);

        for (int id : allIdsList) {
            bool isTrashed = m_paperHub->getTrashed(m_projectIdFilter, id);

            if (m_showTrashedFilter && isTrashed) {
                filteredIdsList.append(id);
            }

            if (m_showNotTrashedFilter && !isTrashed) {
                filteredIdsList.append(id);
            }
        }

        for (int id : filteredIdsList) {
            m_checkedIdsHash.insert(id, Qt::Checked);

            QModelIndex modelIndex = model->getModelIndex(m_projectIdFilter, id).first();

            emit dataChanged(this->mapFromSource(modelIndex),
                             this->mapFromSource(modelIndex),
                             QVector<int>() << Qt::CheckStateRole);
        }
    }
    else {
        for (int paperId : m_paperIdListFilter) {
            m_checkedIdsHash.insert(paperId, Qt::Checked);

            QModelIndex modelIndex =
                    model->getModelIndex(m_projectIdFilter, paperId).first();

            emit dataChanged(this->mapFromSource(modelIndex),
                             this->mapFromSource(modelIndex),
                             QVector<int>() << Qt::CheckStateRole);
        }
    }
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::checkAllButNonPrintable()
{
    SKRPaperListModel *model = static_cast<SKRPaperListModel *>(this->sourceModel());

    if (m_paperIdListFilter.isEmpty()) {
        QList<int> filteredIdsList;
        QList<int> allIdsList = m_paperHub->getAllIds(m_projectIdFilter);

        for (int id : allIdsList) {
            bool isTrashed = m_paperHub->getTrashed(m_projectIdFilter, id);
            bool isPrintable = m_propertyHub->getProperty(m_projectIdFilter, id, "printable", "true") == "true" ? true : false;

            if (m_showTrashedFilter && isTrashed && isPrintable) {
                filteredIdsList.append(id);
            }

            if (m_showNotTrashedFilter && !isTrashed && isPrintable) {
                filteredIdsList.append(id);
            }
        }

        for (int id : filteredIdsList) {
            m_checkedIdsHash.insert(id, Qt::Checked);

            QModelIndex modelIndex = model->getModelIndex(m_projectIdFilter, id).first();

            emit dataChanged(this->mapFromSource(modelIndex),
                             this->mapFromSource(modelIndex),
                             QVector<int>() << Qt::CheckStateRole);
        }
    }
    else {
        for (int paperId : m_paperIdListFilter) {
            bool isPrintable = m_propertyHub->getProperty(m_projectIdFilter, paperId, "printable", "true") == "true" ? true : false;
            if(!isPrintable){
                continue;
            }

            m_checkedIdsHash.insert(paperId, Qt::Checked);

            QModelIndex modelIndex =
                    model->getModelIndex(m_projectIdFilter, paperId).first();

            emit dataChanged(this->mapFromSource(modelIndex),
                             this->mapFromSource(modelIndex),
                             QVector<int>() << Qt::CheckStateRole);
        }
    }
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::checkNone()
{
    SKRPaperListModel *model = static_cast<SKRPaperListModel *>(this->sourceModel());
    QHash<int, Qt::CheckState> checkedIdsHash(m_checkedIdsHash);

    this->clearCheckedList();

    for (int paperId : checkedIdsHash.keys()) {
        QModelIndex modelIndex = model->getModelIndex(m_projectIdFilter, paperId).first();
        emit dataChanged(this->mapFromSource(modelIndex), this->mapFromSource(modelIndex),
                         QVector<int>() << Qt::CheckStateRole);
    }
}

// --------------------------------------------------------------


SKRPaperItem * SKRSearchPaperListProxyModel::getItem(int projectId, int paperId)
{
    SKRPaperListModel *model = static_cast<SKRPaperListModel *>(this->sourceModel());

    SKRPaperItem *item = model->getItem(projectId, paperId);

    return item;
}

// --------------------------------------------------------------


void SKRSearchPaperListProxyModel::loadProjectSettings(int projectId)
{
    QString   unique_identifier = plmdata->projectHub()->getProjectUniqueId(projectId);
    QSettings settings;

    settings.beginGroup("project_" + unique_identifier);

    //    int writeCurrentParent = settings.value("noteCurrentParent",
    // 0).toInt();
    //    this->setParentFilter(projectId, noteCurrentParent);
    settings.endGroup();
}

// --------------------------------------------------------------


void SKRSearchPaperListProxyModel::saveProjectSettings(int projectId)
{
    if (m_projectIdFilter != projectId) {
        return;
    }

    QString   unique_identifier = plmdata->projectHub()->getProjectUniqueId(projectId);
    QSettings settings;

    settings.beginGroup("project_" + unique_identifier);

    //    settings.setValue("noteCurrentParent", m_parentIdFilter);
    settings.endGroup();
}


// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setPaperIdListFilter(
        const QList<int>& paperIdListFilter)
{
    m_paperIdListFilter = paperIdListFilter;

    emit paperIdListFilterChanged(paperIdListFilter);

    this->invalidateFilter();
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setHidePaperIdListFilter(const QList<int> &hidePaperIdListFilter)
{
    m_hidePaperIdListFilter = hidePaperIdListFilter;

    emit hidePaperIdListFilterChanged(hidePaperIdListFilter);

    this->invalidateFilter();
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setForcedCurrentIndex(int forcedCurrentIndex)
{
    m_forcedCurrentIndex = forcedCurrentIndex;
    emit forcedCurrentIndexChanged(m_forcedCurrentIndex);
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setForcedCurrentIndex(int projectId, int paperId)
{
    int forcedCurrentIndex = this->findVisualIndex(projectId, paperId);

    setForcedCurrentIndex(forcedCurrentIndex);
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::setCurrentPaperId(int projectId, int paperId)
{
    if (projectId == -2) {
        return;
    }

    if ((paperId == -2) && (projectId != -2)) {
        this->setParentFilter(projectId, -1);
        return;
    }


    SKRPaperListModel *model = static_cast<SKRPaperListModel *>(this->sourceModel());
    SKRPaperItem *item       = this->getItem(projectId, paperId);

    if (!item) {
        paperId = m_paperHub->getTopPaperId(projectId);
        item    = this->getItem(projectId, paperId);
    }


    if (m_navigateByBranchesEnabled) {
        SKRPaperItem *parentItem = model->getParentPaperItem(item);

        if (parentItem) {
            this->setParentFilter(projectId, parentItem->paperId());
        }
        else {
            this->setParentFilter(projectId, -2); // root item
        }
    }

    this->setForcedCurrentIndex(projectId, paperId);
}

void SKRSearchPaperListProxyModel::setTextFilter(const QString& value)
{
    m_textFilter = value;
    emit textFilterChanged(value);

    this->invalidateFilter();
}

// ----------------------------------------------------------------------------------

QList<int>SKRSearchPaperListProxyModel::getChildrenList(int  projectId,
                                                        int  paperId,
                                                        bool getTrashed,
                                                        bool getNotTrashed)
{
    QList<int> resultChildrenIdList;

    QList<int> allChildrenIdList = m_paperHub->getAllChildren(projectId, paperId);

    for (int childId : allChildrenIdList) {
        bool isTrashed = m_paperHub->getTrashed(projectId, childId);

        if (getTrashed && isTrashed) {
            resultChildrenIdList.append(childId);
        }

        if (getNotTrashed && !isTrashed) {
            resultChildrenIdList.append(childId);
        }
    }

    return resultChildrenIdList;
}

// ----------------------------------------------------------------------------------

QList<int>SKRSearchPaperListProxyModel::getAncestorsList(int  projectId,
                                                         int  paperId,
                                                         bool getTrashed,
                                                         bool getNotTrashed) {
    QList<int> resultAllAncestorsIdList;

    QList<int> allAncestorsIdList =
            m_paperHub->getAllAncestors(projectId, paperId);

    for (int ancestorId : allAncestorsIdList) {
        bool isTrashed = m_paperHub->getTrashed(projectId, ancestorId);

        if (getTrashed && isTrashed) {
            resultAllAncestorsIdList.append(ancestorId);
        }

        if (getNotTrashed && !isTrashed) {
            resultAllAncestorsIdList.append(ancestorId);
        }
    }

    return resultAllAncestorsIdList;
}

// ----------------------------------------------------------------------------------

QList<int>SKRSearchPaperListProxyModel::getSiblingsList(int  projectId,
                                                        int  paperId,
                                                        bool getTrashed,
                                                        bool getNotTrashed) {
    QList<int> resultAllSiblingsIdList;

    QList<int> allSiblingsIdList =
            m_paperHub->getAllSiblings(projectId, paperId);

    for (int ancestorId : allSiblingsIdList) {
        bool isTrashed = m_paperHub->getTrashed(projectId, ancestorId);

        if (getTrashed && isTrashed) {
            resultAllSiblingsIdList.append(ancestorId);
        }

        if (getNotTrashed && !isTrashed) {
            resultAllSiblingsIdList.append(ancestorId);
        }
    }

    return resultAllSiblingsIdList;
}

// ----------------------------------------------------------------------------------


void SKRSearchPaperListProxyModel::setShowNotTrashedFilter(bool showNotTrashedFilter)
{
    m_showNotTrashedFilter = showNotTrashedFilter;

    emit showNotTrashedFilterChanged(showNotTrashedFilter);

    this->invalidateFilter();
}

void SKRSearchPaperListProxyModel::setShowTrashedFilter(bool showTrashedFilter)
{
    m_showTrashedFilter = showTrashedFilter;

    emit showTrashedFilterChanged(showTrashedFilter);

    this->invalidateFilter();
}

// --------------------------------------------------------------

bool SKRSearchPaperListProxyModel::hasChildren(int projectId, int paperId) const
{
    return m_paperHub->hasChildren(projectId,
                                   paperId,
                                   m_showTrashedFilter,
                                   m_showNotTrashedFilter);
}

// --------------------------------------------------------------

int SKRSearchPaperListProxyModel::findVisualIndex(int projectId, int paperId)
{
    int rowCount = this->rowCount(QModelIndex());

    int visualIndex = -2;

    QModelIndex modelIndex;

    for (int i = 0; i < rowCount; ++i) {
        modelIndex = this->index(i, 0);

        if ((this->data(modelIndex,
                        SKRPaperItem::Roles::ProjectIdRole).toInt() == projectId)
                && (this->data(modelIndex,
                               SKRPaperItem::Roles::PaperIdRole).toInt() == paperId)) {
            visualIndex = i;
            break;
        }
    }
    return visualIndex;
}

// --------------------------------------------------------------
QString SKRSearchPaperListProxyModel::getItemName(int projectId, int paperId)
{
    // qDebug() << "getItemName" << projectId << paperId;
    if ((projectId == -2) || (paperId == -2)) {
        return "";
    }
    QString name = "";

    if (paperId == -1) {
        name = plmdata->projectHub()->getProjectName(projectId);
    }
    else {
        SKRPaperItem *item = this->getItem(projectId, paperId);

        if (item) {
            name = item->name();
        }
        else {
            qDebug() << this->metaObject()->className() << "no valid item found";
            name = "";
        }
    }

    return name;
}

// --------------------------------------------------------------

int SKRSearchPaperListProxyModel::getItemIndent(int projectId, int paperId)
{
    if ((projectId == -2) || (paperId == -2)) {
        return -2;
    }
    int indent = -2;

    if (paperId == -1) {
        indent = -1;
    }
    else {
        SKRPaperItem *item = this->getItem(projectId, paperId);

        if (item) {
            indent = item->indent();
        }
        else {
            qDebug() << this->metaObject()->className() << "no valid item found";
            indent = -2;
        }
    }

    return indent;
}

// -----------------------------------------------------------------

SKRResult SKRSearchPaperListProxyModel::addChildItem(int projectId,
                                                     int parentPaperId,
                                                     int visualIndex)
{
    SKRResult result = m_paperHub->addChildPaper(projectId, parentPaperId);

    IFOK(result){
        int newPaperId =  m_paperHub->getLastAddedId();
        result.addData("paperId", newPaperId);


        sort(0);
        emit sortOtherProxyModelsCalled();
        int newVisualIndex = this->findVisualIndex(projectId, newPaperId);

        this->setForcedCurrentIndex(newVisualIndex);
    }
    return result;
}

// -----------------------------------------------------------------

SKRResult SKRSearchPaperListProxyModel::addItemAbove(int projectId,
                                                     int parentPaperId,
                                                     int visualIndex)
{
    SKRResult result = m_paperHub->addPaperAbove(projectId, parentPaperId);

    IFOK(result){
        int newPaperId =  m_paperHub->getLastAddedId();
        result.addData("paperId", newPaperId);


        sort(0);
        emit sortOtherProxyModelsCalled();
        int newVisualIndex = this->findVisualIndex(projectId, newPaperId);

        this->setForcedCurrentIndex(newVisualIndex);
    }
    return result;}

// -----------------------------------------------------------------

SKRResult SKRSearchPaperListProxyModel::addItemBelow(int projectId,
                                                     int parentPaperId,
                                                     int visualIndex)
{
    SKRResult result = m_paperHub->addPaperBelow(projectId, parentPaperId);
    IFOK(result){
        int newPaperId =  m_paperHub->getLastAddedId();
        result.addData("paperId", newPaperId);


        sort(0);
        emit sortOtherProxyModelsCalled();
        int newVisualIndex = this->findVisualIndex(projectId, newPaperId);

        this->setForcedCurrentIndex(newVisualIndex);
    }
    return result;

}

// --------------------------------------------------------------

SKRResult SKRSearchPaperListProxyModel::moveUp(int projectId, int paperId, int visualIndex)
{
    SKRPaperItem *item = this->getItem(projectId, paperId);

    if (!item) {
        return SKRResult();
    }
    SKRResult result = m_paperHub->movePaperUp(projectId, paperId);
    IFOK(result){
        sort(0);
        emit sortOtherProxyModelsCalled();
        int newVisualIndex = this->findVisualIndex(projectId, paperId);

        this->setForcedCurrentIndex(newVisualIndex);
    }
    return result;
}

// --------------------------------------------------------------

SKRResult SKRSearchPaperListProxyModel::moveDown(int projectId, int paperId, int visualIndex)
{
    SKRPaperItem *item = this->getItem(projectId, paperId);

    if (!item) {
        return SKRResult();
    }
    SKRResult result = m_paperHub->movePaperDown(projectId, paperId);
    IFOK(result){
        sort(0);
        emit sortOtherProxyModelsCalled();
        int newVisualIndex = this->findVisualIndex(projectId, paperId);

        this->setForcedCurrentIndex(newVisualIndex);
    }
    return result;
}

// --------------------------------------------------------------


///
/// \brief SKRSearchPaperListProxyModel::moveItem
/// \param from source item index number
/// \param to target item index number
/// Carefull, this is only used for manually moving a visual item
void SKRSearchPaperListProxyModel::moveItem(int from, int to) {




    if (from == to) return;

    int modelFrom = from;
    int modelTo   = to + (from < to ? 1 : 0);


    QModelIndex fromIndex = this->index(modelFrom, 0);
    int fromPaperId       =
            this->data(fromIndex, SKRPaperItem::Roles::PaperIdRole).toInt();
    int fromProjectId =
            this->data(fromIndex, SKRPaperItem::Roles::ProjectIdRole).toInt();

    QModelIndex toIndex = this->index(modelTo, 0);
    int toPaperId       = this->data(toIndex, SKRPaperItem::Roles::PaperIdRole).toInt();
    int toProjectId     = this->data(toIndex, SKRPaperItem::Roles::ProjectIdRole).toInt();
    int toSortOrder     = this->data(toIndex, SKRPaperItem::Roles::SortOrderRole).toInt();

    qDebug() << "fromPaperId : " << fromPaperId << this->data(fromIndex,
                                                              SKRPaperItem::Roles::NameRole)
                .toString();
    qDebug() << "toPaperId : " << toPaperId << this->data(toIndex,
                                                          SKRPaperItem::Roles::NameRole).
                toString();


    m_paperHub->movePaper(fromProjectId, fromPaperId, toPaperId, false);


    sort(0);
    emit sortOtherProxyModelsCalled();
    this->invalidate();

}

// --------------------------------------------------------------

int SKRSearchPaperListProxyModel::goUp()
{
    SKRPaperListModel *model = static_cast<SKRPaperListModel *>(this->sourceModel());
    SKRPaperItem *parentItem = getItem(m_projectIdFilter, m_parentIdFilter);

    if (!parentItem) {
        return -2;
    }
    SKRPaperItem *grandParentItem = model->getParentPaperItem(parentItem);

    int grandParentId = -2;

    if (grandParentItem) {
        grandParentId = grandParentItem->paperId();
    }
    this->setParentFilter(m_projectIdFilter, grandParentId);

    return grandParentId;
}

// --------------------------------------------------------------
void SKRSearchPaperListProxyModel::setParentFilter(int projectId, int parentId)
{
    m_projectIdFilter = projectId;
    m_parentIdFilter  = parentId;
    emit parentIdFilterChanged(m_parentIdFilter);
    emit projectIdFilterChanged(m_projectIdFilter);

    this->invalidateFilter();
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::trashItemWithChildren(int projectId, int paperId)
{
    SKRPaperItem *item = this->getItem(projectId, paperId);

    if (!item) {
        return;
    }

    m_paperHub->setTrashedWithChildren(projectId, paperId, true);
}

// --------------------------------------------------------------

QHash<int, QByteArray>SKRSearchPaperListProxyModel::roleNames() const {
    QHash<int, QByteArray> roles = QSortFilterProxyModel::roleNames();

    roles[Qt::CheckStateRole] = "checkState";

    return roles;
}

// --------------------------------------------------------------

int SKRSearchPaperListProxyModel::getLastOfHistory(int projectId)
{
    QList<int> list = m_historyList.value(projectId, QList<int>());

    if (list.isEmpty()) {
        return -2;
    }

    return list.last();
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::removeLastOfHistory(int projectId)
{
    QList<int> list = m_historyList.value(projectId, QList<int>());

    if (list.isEmpty()) {
        return;
    }

    list.removeLast();
    m_historyList.insert(projectId, list);
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::clearHistory(int projectId)
{
    m_historyList.remove(projectId);
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::addHistory(int projectId, int paperId)
{
    QList<int> list = m_historyList.value(projectId, QList<int>());

    list.append(paperId);
    m_historyList.insert(projectId, list);
}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::cut(int projectId, int paperId){

}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::copy(int projectId, int paperId){

}

// --------------------------------------------------------------

void SKRSearchPaperListProxyModel::paste(int targetProjectId, int targetParentId){

}
