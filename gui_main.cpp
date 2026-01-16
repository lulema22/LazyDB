#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/grid.h>
#include <wx/filepicker.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/dirdlg.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include "db/Database.h"
#include "db/DbErrors.h"

static std::vector<std::string> Split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) parts.push_back(item);
    return parts;
}

static std::string JoinRow(const std::vector<std::string>& cells, char delim=';') {
    std::string out;
    for (size_t i = 0; i < cells.size(); ++i) {
        if (i) out.push_back(delim);
        out += cells[i];
    }
    return out;
}

struct TableSpec {
    wxString title;
    wxString defaultFilename;
    std::vector<wxString> columns;
};

class CsvTablePanel : public wxPanel {
public:
    CsvTablePanel(wxWindow* parent, TableSpec spec)
        : wxPanel(parent), spec_(std::move(spec)) {

        auto* vbox = new wxBoxSizer(wxVERTICAL);

        auto* top = new wxBoxSizer(wxHORIZONTAL);
        picker_ = new wxFilePickerCtrl(this, wxID_ANY, wxEmptyString,"Choose CSV file", "*.csv",wxDefaultPosition, wxDefaultSize,wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);

        loadBtn_ = new wxButton(this, wxID_ANY, "Load");
        saveAsBtn_ = new wxButton(this, wxID_ANY, "Save As...");

        top->Add(new wxStaticText(this, wxID_ANY, spec_.title), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10);
        top->Add(picker_, 1, wxRIGHT, 10);
        top->Add(loadBtn_, 0, wxRIGHT, 10);
        top->Add(saveAsBtn_, 0);

        vbox->Add(top, 0, wxEXPAND|wxALL, 8);

        grid_ = new wxGrid(this, wxID_ANY);
        grid_->CreateGrid(0, (int)spec_.columns.size());
        for (int c = 0; c < (int)spec_.columns.size(); ++c) {
            grid_->SetColLabelValue(c, spec_.columns[c]);
        }
        grid_->EnableEditing(true);
        grid_->SetRowLabelSize(60);

        vbox->Add(grid_, 1, wxEXPAND|wxLEFT|wxRIGHT| wxBOTTOM, 8);

        SetSizer(vbox);

        loadBtn_->Bind(wxEVT_BUTTON, &CsvTablePanel::OnLoad, this);
        saveAsBtn_->Bind(wxEVT_BUTTON, &CsvTablePanel::OnSaveAs, this);
    }

    wxGrid* GetGrid() const {return grid_; }
    const TableSpec& GetSpec() const {return spec_; }
    wxString GetPath() const {return picker_->GetPath(); }
    wxString GetDefaultFilename() const {return spec_.defaultFilename; }

    void SetDefaultPath(const wxString& fullPath) {
        picker_->SetPath(fullPath);
    }

    std::vector<std::string> ToCsvLines() const {
        std::vector<std::string> lines;
        const int rows = grid_->GetNumberRows();
        const int cols = grid_->GetNumberCols();

        for (int r=0; r < rows; ++r) {
            std::vector<std::string> cells;
            cells.reserve(cols);

            bool allEmpty = true;
            for (int c = 0; c < cols; ++c) {
                wxString v = grid_->GetCellValue(r, c);
                std::string s = v.ToStdString();
                if (!s.empty()) allEmpty = false;
                cells.push_back(std::move(s));
            }
            if (!allEmpty) {
                lines.push_back(JoinRow(cells, ';'));
            }
        }
        return lines;
    }

    void LoadFromFile(const wxString& path) {
        std::ifstream in(path.ToStdString());
        if (!in) throw std::runtime_error("Cannot open file: " + path.ToStdString());

        std::vector<std::vector<std::string>> rows;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto parts = Split(line, ';');
            if ((int)parts.size() != (int)spec_.columns.size()) {
                throw std::runtime_error(
                    "Bad CSV columns count in " + path.ToStdString() +
                    " (expected " + std::to_string(spec_.columns.size()) +
                    ", got " + std::to_string(parts.size()) + ")"
                );
            }
            rows.push_back(std::move(parts));
        }

        const int oldRows = grid_->GetNumberRows();
        if (oldRows > 0) grid_->DeleteRows(0, oldRows);

        if (!rows.empty()) {
            grid_->AppendRows((int)rows.size());
            for (int r = 0; r < (int)rows.size(); ++r) {
                for (int c = 0; c < (int)rows[r].size(); ++c) {
                    grid_->SetCellValue(r, c, rows[r][c]);
                }
            }
        }
        grid_->AutoSizeColumns(false);
    }

private:
    static void WriteFileAtomic(const wxString& path, const std::vector<std::string>& lines) {
        wxString tmpPath = path + ".tmp";
        {
            std::ofstream out(tmpPath.ToStdString());
            if (!out) throw std::runtime_error("Cannot write file: " + tmpPath.ToStdString());
            for (const auto& l : lines) out << l << "\n";
        }
        if (wxFileExists(path)) {
            if (!wxRemoveFile(path)) {
                throw std::runtime_error("Cannot overwrite file: " + path.ToStdString());
            }
        }

        if (!wxRenameFile(tmpPath, path)) {
            throw std::runtime_error("Cannot finalize file: " + path.ToStdString());
        }
    }

    void OnLoad(wxCommandEvent&) {
        const wxString path = picker_->GetPath();
        if (path.IsEmpty()) return;
        try {
            LoadFromFile(path);
            wxMessageBox("Loaded OK", spec_.title, wxOK | wxICON_INFORMATION, this);
        } catch (const std::exception& e) {
            wxMessageBox(e.what(), "Load error", wxOK | wxICON_ERROR, this);
        }
    }

    void OnSaveAs(wxCommandEvent&) {
        wxFileDialog saveDlg(this, "Save CSV As", "", spec_.defaultFilename,
                             "CSV files (*.csv)|*.csv",
                             wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (saveDlg.ShowModal() != wxID_OK) return;

        try {
            auto lines = ToCsvLines();
            WriteFileAtomic(saveDlg.GetPath(), lines);
            wxMessageBox("Saved OK", "Save", wxOK | wxICON_INFORMATION, this);
        } catch (const std::exception& e) {
            wxMessageBox(e.what(), "Save error", wxOK | wxICON_ERROR, this);
        }
    }

private:
    TableSpec spec_;
    wxFilePickerCtrl* picker_{nullptr};
    wxButton* loadBtn_{nullptr};
    wxButton* saveAsBtn_{nullptr};
    wxGrid* grid_{nullptr};
};

class MainFrame : public wxFrame {
public:
    MainFrame()
        : wxFrame(nullptr, wxID_ANY, "LazyDB GUI (Validate + Save + Add/Delete + Highlight)",
                  wxDefaultPosition, wxSize(1250, 820)) {

        auto* root = new wxBoxSizer(wxVERTICAL);

        auto* top = new wxBoxSizer(wxHORIZONTAL);
        validateBtn_= new wxButton(this, wxID_ANY, "Validate (FK/UNIQUE)");
        addRowBtn_ = new wxButton(this, wxID_ANY, "Add Row");
        deleteRowBtn_ = new wxButton(this, wxID_ANY, "Delete Selected");
        saveAllAsBtn_ = new wxButton(this, wxID_ANY, "Save All As...");
        reloadAllBtn_ = new wxButton(this, wxID_ANY, "Reload All");

        top->Add(validateBtn_,0,wxALL, 6);
        top->Add(addRowBtn_,0, wxALL, 6);
        top->Add(deleteRowBtn_,0, wxALL, 6);
        top->Add(saveAllAsBtn_,0,wxALL,6);
        top->Add(reloadAllBtn_,0, wxALL, 6);

        root->Add(top, 0, wxEXPAND);

        // -------- Search bar (active tab) --------
        auto* search = new wxBoxSizer(wxHORIZONTAL);

        search->Add(new wxStaticText(this, wxID_ANY, "Search:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        searchField_ = new wxChoice(this, wxID_ANY);
        search->Add(searchField_, 0, wxRIGHT, 10);

        searchL1_ = new wxStaticText(this, wxID_ANY, "Value:");
        search->Add(searchL1_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

        searchV1_ = new wxTextCtrl(this, wxID_ANY);
        search->Add(searchV1_, 0, wxRIGHT, 10);

        searchL2_ = new wxStaticText(this, wxID_ANY, "To:");
        search->Add(searchL2_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

        searchV2_ = new wxTextCtrl(this, wxID_ANY);
        search->Add(searchV2_, 0, wxRIGHT, 10);

        searchBtn_ = new wxButton(this, wxID_ANY, "Search");
        clearSearchBtn_ = new wxButton(this, wxID_ANY, "Clear");
        search->Add(searchBtn_, 0, wxRIGHT, 8);
        search->Add(clearSearchBtn_, 0);

        root->Add(search, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

        notebook_ = new wxNotebook(this, wxID_ANY);

        panels_.push_back(new CsvTablePanel(notebook_, {
            "Addresses", "addresses.csv",
            {"id","city","street","building","type"}
        }));
        panels_.push_back(new CsvTablePanel(notebook_, {
            "Departments", "departments.csv",
            {"id","name","address_id"}
        }));
        panels_.push_back(new CsvTablePanel(notebook_, {
            "Employees", "employees.csv",
            {"id","last","first","middle","birth_year","dept_id"}
        }));
        panels_.push_back(new CsvTablePanel(notebook_, {
            "Suppliers", "suppliers.csv",
            {"id","name","city","phone","email"}
        }));
        panels_.push_back(new CsvTablePanel(notebook_, {
            "Products", "products.csv",
            {"id","name","category","unit","default_supplier_id"}
        }));
        panels_.push_back(new CsvTablePanel(notebook_, {
            "Purchases", "purchases.csv",
            {"id","date","dept_id","supplier_id","product_id","qty","unit_price"}
        }));

        notebook_->AddPage(panels_[0], "Addresses", false);
        notebook_->AddPage(panels_[1], "Departments", false);
        notebook_->AddPage(panels_[2], "Employees", false);
        notebook_->AddPage(panels_[3], "Suppliers", false);
        notebook_->AddPage(panels_[4], "Products", false);
        notebook_->AddPage(panels_[5], "Purchases", false);

        root->Add(notebook_, 1, wxEXPAND | wxLEFT | wxRIGHT, 6);

        root->Add(new wxStaticText(this, wxID_ANY, "Log:"), 0, wxLEFT | wxTOP, 8);
        log_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 180),
                              wxTE_MULTILINE | wxTE_READONLY);
        root->Add(log_, 0, wxEXPAND | wxALL, 6);

        SetSizer(root);

        wxString dataDir = wxFileName::DirName("../data").GetFullPath();
        panels_[0]->SetDefaultPath(dataDir+ "/addresses.csv");
        panels_[1]->SetDefaultPath(dataDir+ "/departments.csv");
        panels_[2]->SetDefaultPath(dataDir+ "/employees.csv");
        panels_[3]->SetDefaultPath(dataDir + "/suppliers.csv");
        panels_[4]->SetDefaultPath(dataDir+ "/products.csv");
        panels_[5]->SetDefaultPath(dataDir + "/purchases.csv");

        validateBtn_->Bind(wxEVT_BUTTON, &MainFrame::OnValidate, this);
        addRowBtn_->Bind(wxEVT_BUTTON, &MainFrame::OnAddRow, this);
        deleteRowBtn_->Bind(wxEVT_BUTTON, &MainFrame::OnDeleteSelected, this);
        saveAllAsBtn_->Bind(wxEVT_BUTTON, &MainFrame::OnSaveAllAs, this);
        reloadAllBtn_->Bind(wxEVT_BUTTON, &MainFrame::OnReloadAll, this);

        searchBtn_->Bind(wxEVT_BUTTON, &MainFrame::OnSearch, this);
        clearSearchBtn_->Bind(wxEVT_BUTTON, &MainFrame::OnClearSearch, this);
        searchField_->Bind(wxEVT_CHOICE, &MainFrame::OnSearchFieldChanged, this);
        notebook_->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &MainFrame::OnTabChanged, this);

        UpdateSearchUIForTab(notebook_->GetSelection());

        for (auto* p : panels_) {
            if (p && p->GetGrid()) {
                p->GetGrid()->Bind(wxEVT_GRID_CELL_CHANGED, &MainFrame::OnAnyGridEdited, this);
            }
        }

        UpdateSaveButtons();
    }

private:
    void AppendLog(const wxString& msg) {
        if (log_) log_->AppendText(msg + "\n");
    }

    void UpdateSaveButtons() {
        if (saveAllAsBtn_) saveAllAsBtn_->Enable(validatedOk_);
    }

    void OnAnyGridEdited(wxGridEvent& evt) {
        validatedOk_ = false;
        ClearLastHighlight();
        UpdateSaveButtons();
        evt.Skip();
    }

    void CommitAllEdits() {
        for (auto* p : panels_) {
            if (!p) continue;
            wxGrid* g = p->GetGrid();
            if (!g) continue;
            g->DisableCellEditControl();
        }
    }

    int TableNameToTabIndex(const std::string& table) const {
        if (table == "addresses")return 0;
        if (table == "departments") return 1;
        if (table == "employees")return 2;
        if (table == "suppliers")return 3;
        if (table == "products")return 4;
        if (table == "purchases")return 5;
        return -1;
    }

    int FindColumnIndex(CsvTablePanel* panel, const std::string& field) const {
        if (!panel) return -1;
        const auto& cols = panel->GetSpec().columns;
        for (int i = 0; i < (int)cols.size(); ++i) {
            if (cols[i].ToStdString() == field) return i;
        }
        return -1;
    }

    void ClearLastHighlight() {
        if (lastTab_ < 0) return;
        if (lastTab_ >= (int)panels_.size()) return;

        wxGrid* g = panels_[lastTab_]->GetGrid();
        if (!g) return;
        if (lastRow_ < 0 || lastCol_ < 0) return;
        if (lastRow_ >= g->GetNumberRows() || lastCol_ >= g->GetNumberCols()) return;

        g->SetCellBackgroundColour(lastRow_, lastCol_, lastOldColor_);
        g->ForceRefresh();

        lastTab_ = lastRow_ = lastCol_ = -1;
    }

    void HighlightErrorCell(const DbConstraintError& e) {
        ClearLastHighlight();

        int tab = TableNameToTabIndex(e.GetTable());
        if (tab < 0 || tab >= (int)panels_.size()) return;

        int row = e.GetRowIndex();
        int col = FindColumnIndex(panels_[tab], e.GetField());
        if (row < 0 || col < 0) return;

        wxGrid* grid = panels_[tab]->GetGrid();
        if (!grid) return;

        if (row >= grid->GetNumberRows() || col >= grid->GetNumberCols()) return;

        notebook_->SetSelection(tab);

        lastTab_ = tab;
        lastRow_ = row;
        lastCol_ = col;
        lastOldColor_ = grid->GetCellBackgroundColour(row, col);

        grid->MakeCellVisible(row, col);
        grid->SetGridCursor(row, col);
        grid->SelectBlock(row, col, row, col);

        grid->SetCellBackgroundColour(row, col, wxColour(255, 220, 220));
        grid->ForceRefresh();
    }

    static wxString EnsureTempDir() {
        wxString tmp = wxStandardPaths::Get().GetTempDir();
        wxString dir = tmp + "/lazydb_validate";
        wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
        return dir;
    }

    static wxString WriteTempCsv(const wxString& dir, const wxString& name, const std::vector<std::string>& lines) {
        wxString path = dir + "/" + name;
        std::ofstream out(path.ToStdString());
        if (!out) throw std::runtime_error("Cannot write temp file: " + path.ToStdString());
        for (const auto& l : lines) out << l << "\n";
        return path;
    }

    static void WriteFileAtomic(const wxString& path, const std::vector<std::string>& lines) {
        wxString tmpPath = path + ".tmp";

        {
            std::ofstream out(tmpPath.ToStdString());
            if (!out) throw std::runtime_error("Cannot write file: " + tmpPath.ToStdString());
            for (const auto& l : lines) out << l << "\n";
        }

        if (wxFileExists(path)) {
            if (!wxRemoveFile(path)) {
                throw std::runtime_error("Cannot overwrite file: " + path.ToStdString());
            }
        }

        if (!wxRenameFile(tmpPath, path)) {
            throw std::runtime_error("Cannot finalize file: " + path.ToStdString());
        }
    }

    bool RowAllEmpty(wxGrid* g, int r) const {
        if (!g) return true;
        for (int c = 0; c < g->GetNumberCols(); ++c) {
            if (!g->GetCellValue(r, c).IsEmpty()) return false;
        }
        return true;
    }

    void RequireNonEmptyFieldsOrThrow(int tab, const std::vector<int>& requiredCols) {
        wxGrid* g = panels_[tab]->GetGrid();
        if (!g) return;

        for (int r = 0; r < g->GetNumberRows(); ++r) {
            if (RowAllEmpty(g, r)) continue;

            for (int col : requiredCols) {
                if (col < 0 || col >= g->GetNumberCols()) continue;
                if (g->GetCellValue(r, col).IsEmpty()) {
                    notebook_->SetSelection(tab);
                    ClearLastHighlight();
                    lastTab_ = tab;
                    lastRow_ = r;
                    lastCol_ = col;
                    lastOldColor_ = g->GetCellBackgroundColour(r, col);

                    g->MakeCellVisible(r, col);
                    g->SetGridCursor(r, col);
                    g->SelectBlock(r, col, r, col);
                    g->SetCellBackgroundColour(r, col, wxColour(255, 220, 220));
                    g->ForceRefresh();

                    throw std::runtime_error(
                        "Required cell is empty: table=" +
                        panels_[tab]->GetSpec().title.ToStdString() +
                        " row=" + std::to_string(r) +
                        " col=" + panels_[tab]->GetSpec().columns[col].ToStdString()
                    );
                }
            }
        }
    }

    Database BuildDbFromGridsOrThrow() {
        CommitAllEdits();

        RequireNonEmptyFieldsOrThrow(0,{0,1,2,3,4});  // addresses
        RequireNonEmptyFieldsOrThrow(1,{0,1,2});   // departments
        RequireNonEmptyFieldsOrThrow(2, {0,1,2,3,4,5});    // employees
        RequireNonEmptyFieldsOrThrow(3, {0,1,2,3,4});        // suppliers
        RequireNonEmptyFieldsOrThrow(4, {0,1,2,3,4});      // products
        RequireNonEmptyFieldsOrThrow(5, {0,1,2,3,4,5,6});  // purchases

        const wxString dir = EnsureTempDir();

        wxString addrPath = WriteTempCsv(dir, "addresses.csv",panels_[0]->ToCsvLines());
        wxString deptPath = WriteTempCsv(dir, "departments.csv",panels_[1]->ToCsvLines());
        wxString empPath  = WriteTempCsv(dir, "employees.csv", panels_[2]->ToCsvLines());
        wxString supPath  = WriteTempCsv(dir, "suppliers.csv", panels_[3]->ToCsvLines());
        wxString prodPath = WriteTempCsv(dir, "products.csv", panels_[4]->ToCsvLines());
        wxString purPath  = WriteTempCsv(dir, "purchases.csv", panels_[5]->ToCsvLines());

        return Database::LoadFromFiles(
            addrPath.ToStdString(),
            deptPath.ToStdString(),
            empPath.ToStdString(),
            supPath.ToStdString(),
            prodPath.ToStdString(),
            purPath.ToStdString()
        );
    }

    static int GetSelectedRow(wxGrid* g) {
        if (!g) return -1;
        wxArrayInt rows = g->GetSelectedRows();
        if (!rows.IsEmpty()) return rows[0];
        return g->GetGridCursorRow();
    }

    static int GetIdFromRow(wxGrid* g, int row) {
        if (!g || row < 0 || row >= g->GetNumberRows()) return -1;
        wxString idStr = g->GetCellValue(row, 0);
        long v = -1;
        if (!idStr.ToLong(&v)) return -1;
        return (int)v;
    }

    int GetMaxIdInTab(int tab) const {
        wxGrid* g = panels_[tab]->GetGrid();
        if (!g) return 0;
        int maxId = 0;
        for (int r = 0; r < g->GetNumberRows(); ++r) {
            if (RowAllEmpty(g, r)) continue;
            long v = 0;
            wxString s = g->GetCellValue(r, 0);
            if (s.ToLong(&v)) maxId = std::max(maxId, (int)v);
        }
        return maxId;
    }

    int GetFirstIdInTab(int tab) const {
        wxGrid* g = panels_[tab]->GetGrid();
        if (!g) return -1;
        for (int r = 0; r < g->GetNumberRows(); ++r) {
            if (RowAllEmpty(g, r)) continue;
            long v = -1;
            wxString s = g->GetCellValue(r, 0);
            if (s.ToLong(&v)) return (int)v;
        }
        return -1;
    }

    void OnAddRow(wxCommandEvent&) {
        int tab = notebook_->GetSelection();
        if (tab < 0 || tab >= (int)panels_.size()) return;

        CommitAllEdits();

        wxGrid* g = panels_[tab]->GetGrid();
        if (!g) return;

        int newId = GetMaxIdInTab(tab) + 1;

        g->AppendRows(1);
        int r = g->GetNumberRows() - 1;

        g->SetCellValue(r, 0, wxString::Format("%d", newId));

        if (tab == 0) { // addresses
            g->SetCellValue(r, 1, "City");
            g->SetCellValue(r, 2, "Street");
            g->SetCellValue(r, 3, "1");
            g->SetCellValue(r, 4, "Office");
        } else if (tab == 1) { // departments
            int addrId = GetFirstIdInTab(0);
            if (addrId < 0) addrId = 1;
            g->SetCellValue(r, 1, "NewDepartment");
            g->SetCellValue(r, 2, wxString::Format("%d", addrId));
        } else if (tab == 2) { // employees
            int deptId = GetFirstIdInTab(1);
            if (deptId < 0) deptId = 1;
            g->SetCellValue(r, 1, "Last");
            g->SetCellValue(r, 2, "First");
            g->SetCellValue(r, 3, "Middle");
            g->SetCellValue(r, 4, "2000");
            g->SetCellValue(r, 5, wxString::Format("%d", deptId));
        } else if (tab == 3) { // suppliers
            g->SetCellValue(r, 1, "NewSupplier");
            g->SetCellValue(r, 2, "City");
            g->SetCellValue(r, 3, "+000000000");
            g->SetCellValue(r, 4, "mail@example.com");
        } else if (tab == 4) { // products
            int supId = GetFirstIdInTab(3);
            if (supId < 0) supId = 1;
            g->SetCellValue(r, 1, "NewProduct");
            g->SetCellValue(r, 2, "Category");
            g->SetCellValue(r, 3, "pcs");
            g->SetCellValue(r, 4, wxString::Format("%d", supId));
        } else if (tab == 5) { // purchases
            int deptId = GetFirstIdInTab(1); if (deptId < 0) deptId = 1;
            int supId  = GetFirstIdInTab(3); if (supId < 0) supId = 1;
            int prId   = GetFirstIdInTab(4); if (prId < 0) prId = 1;
            g->SetCellValue(r, 1, "2025-01-01");
            g->SetCellValue(r, 2, wxString::Format("%d", deptId));
            g->SetCellValue(r, 3, wxString::Format("%d", supId));
            g->SetCellValue(r, 4, wxString::Format("%d", prId));
            g->SetCellValue(r, 5, "1");
            g->SetCellValue(r, 6, "1.00");
        }

        validatedOk_ = false;
        ClearLastHighlight();
        UpdateSaveButtons();

        g->MakeCellVisible(r, 0);
        g->SetGridCursor(r, 0);
        g->SelectBlock(r, 0, r, 0);

        AppendLog(wxString::Format("Added row: tab=%d id=%d", tab, newId));
    }

    void OnDeleteSelected(wxCommandEvent&) {
        int tab = notebook_->GetSelection();
        if (tab < 0 || tab >= (int)panels_.size()) return;

        wxGrid* g = panels_[tab]->GetGrid();
        if (!g) return;

        CommitAllEdits();

        int row = GetSelectedRow(g);
        if (row < 0 || row >= g->GetNumberRows()) {
            wxMessageBox("Select a row first.", "Delete", wxOK | wxICON_WARNING, this);
            return;
        }

        int id = GetIdFromRow(g, row);
        if (id < 0) {
            wxMessageBox("Bad or empty id in selected row (column 0).", "Delete", wxOK | wxICON_WARNING, this);
            return;
        }

        try {
            Database db = BuildDbFromGridsOrThrow();

            switch (tab) {
                case 0: db.DeleteAddress(id); break;
                case 1: db.DeleteDepartment(id); break;
                case 2: db.DeleteEmployee(id); break;
                case 3: db.DeleteSupplier(id); break;
                case 4: db.DeleteProduct(id); break;
                case 5: db.DeletePurchase(id); break;
                default:
                    wxMessageBox("Unknown table tab.", "Delete", wxOK | wxICON_ERROR, this);
                    return;
            }

            g->DeleteRows(row, 1);

            validatedOk_ = false;
            ClearLastHighlight();
            UpdateSaveButtons();

            AppendLog(wxString::Format("Deleted id=%d from tab=%d", id, tab));

        } catch (const DbConstraintError& e) {
            validatedOk_ = false;
            UpdateSaveButtons();
            AppendLog(wxString::Format("Constraint error: %s", e.what()));
            HighlightErrorCell(e);
            wxMessageBox(e.what(), "Delete blocked", wxOK | wxICON_ERROR, this);

        } catch (const std::exception& e) {
            validatedOk_ = false;
            UpdateSaveButtons();
            AppendLog(wxString::Format("Error: %s", e.what()));
            wxMessageBox(e.what(), "Delete failed", wxOK | wxICON_ERROR, this);
        }
    }

    void OnValidate(wxCommandEvent&) {
        try {
            Database db = BuildDbFromGridsOrThrow();

            ClearLastHighlight();
            validatedOk_ = true;
            UpdateSaveButtons();

            wxString okMsg;
            okMsg << "VALID OK. Rows: "
                  << "addresses=" << db.Addresses().GetRowCount() << ", "
                  << "departments=" << db.Departments().GetRowCount() << ", "
                  << "employees=" << db.Employees().GetRowCount() << ", "
                  << "suppliers=" << db.Suppliers().GetRowCount() << ", "
                  << "products=" << db.Products().GetRowCount() << ", "
                  << "purchases=" << db.Purchases().GetRowCount();

            AppendLog(okMsg);
            wxMessageBox("Validation OK \nNow you can use 'Save All As...'", "Validate",
                         wxOK | wxICON_INFORMATION, this);

        } catch (const DbConstraintError& e) {
            validatedOk_ = false;
            UpdateSaveButtons();

            AppendLog(wxString::Format("Constraint error: %s", e.what()));
            HighlightErrorCell(e);

            wxMessageBox(e.what(), "Validate failed", wxOK | wxICON_ERROR, this);

        } catch (const std::exception& e) {
            validatedOk_ = false;
            UpdateSaveButtons();

            AppendLog(wxString::Format("Error: %s", e.what()));
            wxMessageBox(e.what(), "Validate failed", wxOK | wxICON_ERROR, this);
        }
    }

    void OnSaveAllAs(wxCommandEvent&) {
        CommitAllEdits();

        if (!validatedOk_) {
            wxMessageBox("Please run Validate first (and fix errors) before saving.",
                         "Save All As...", wxOK | wxICON_WARNING, this);
            return;
        }

        wxDirDialog dlg(this, "Choose folder to save ALL CSV files", "",
                        wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) return;

        try {
            wxString outDir = dlg.GetPath();
            wxFileName::Mkdir(outDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

            for (auto* p : panels_) {
                if (!p) continue;
                wxString outPath = outDir + "/" + p->GetDefaultFilename();
                WriteFileAtomic(outPath, p->ToCsvLines());
            }

            AppendLog("Saved ALL tables into: " + outDir);
            wxMessageBox("Saved ✅", "Save All As...", wxOK | wxICON_INFORMATION, this);

        } catch (const std::exception& e) {
            AppendLog(wxString::Format("Save failed: %s", e.what()));
            wxMessageBox(e.what(), "Save All As... failed", wxOK | wxICON_ERROR, this);
        }
    }

    void OnReloadAll(wxCommandEvent&) {
        CommitAllEdits();

        try {
            for (auto* p : panels_) {
                if (!p) continue;
                wxString path = p->GetPath();
                if (path.IsEmpty()) continue;
                p->LoadFromFile(path);
            }

            validatedOk_ = false;
            ClearLastHighlight();
            UpdateSaveButtons();

            AppendLog("Reloaded all tables from selected paths.");
            wxMessageBox("Reloaded ✅", "Reload All", wxOK | wxICON_INFORMATION, this);

        } catch (const std::exception& e) {
            validatedOk_ = false;
            UpdateSaveButtons();

            AppendLog(wxString::Format("Reload failed: %s", e.what()));
            wxMessageBox(e.what(), "Reload All failed", wxOK | wxICON_ERROR, this);
        }
    }

private:

    enum class SearchKind { Disabled, ExactString, ExactInt, RangeInt, RangeString };

    SearchKind GetSearchKind(int tab, const wxString& field) const {
        // tabs: 0 Addresses, 1 Departments, 2 Employees, 3 Suppliers, 4 Products, 5 Purchases
        if (tab == 0) { // Addresses
            if (field == "city") return SearchKind::ExactString;
            if (field == "id_range") return SearchKind::RangeInt;
        } else if (tab == 1) { // Departments
            if (field == "name") return SearchKind::ExactString;
            if (field == "address_id") return SearchKind::ExactInt;
        } else if (tab == 2) { // Employees
            if (field == "full_name") return SearchKind::ExactString;
            if (field == "birth_year") return SearchKind::RangeInt;
            if (field == "dept_id") return SearchKind::ExactInt;
        } else if (tab == 3) { // Suppliers
            if (field == "name") return SearchKind::ExactString;
            if (field == "city") return SearchKind::ExactString;
        } else if (tab == 4) { // Products
            if (field == "name") return SearchKind::ExactString;
            if (field == "default_supplier_id") return SearchKind::ExactInt;
        } else if (tab == 5) { // Purchases
            if (field == "date") return SearchKind::RangeString;
            if (field == "supplier_id") return SearchKind::ExactInt;
            if (field == "product_id") return SearchKind::ExactInt;
            if (field == "dept_id") return SearchKind::ExactInt;
        }
        return SearchKind::Disabled;
    }

    void SetRangeVisible(bool visible) {
        if (!searchL2_ || !searchV2_) return;
        searchL2_->Show(visible);
        searchV2_->Show(visible);
    }

    void ApplyFieldUI(int tab) {
        const wxString field = searchField_ ? searchField_->GetStringSelection() : "";
        const SearchKind kind = GetSearchKind(tab, field);

        if (kind == SearchKind::Disabled) {
            searchL1_->SetLabel("Value:");
            SetRangeVisible(false);
            searchV1_->Enable(false);
            if (searchV2_) searchV2_->Enable(false);
            searchBtn_->Enable(false);
            Layout();
            return;
        }

        searchV1_->Enable(true);
        searchBtn_->Enable(true);

        if (kind == SearchKind::ExactString) {
            searchL1_->SetLabel("Value:");
            SetRangeVisible(false);
        } else if (kind == SearchKind::ExactInt) {
            searchL1_->SetLabel("Value:");
            SetRangeVisible(false);
        } else if (kind == SearchKind::RangeInt) {
            searchL1_->SetLabel("From:");
            searchL2_->SetLabel("To:");
            SetRangeVisible(true);
            searchV2_->Enable(true);
        } else if (kind == SearchKind::RangeString) {
            searchL1_->SetLabel("From:");
            searchL2_->SetLabel("To:");
            SetRangeVisible(true);
            searchV2_->Enable(true);
        }

        Layout();
    }

    void UpdateSearchUIForTab(int tab) {
        if (!searchField_) return;

        searchField_->Clear();
        searchV1_->Clear();
        if (searchV2_) searchV2_->Clear();

        // tabs: 0 Addresses, 1 Departments, 2 Employees, 3 Suppliers, 4 Products, 5 Purchases
        if (tab == 0) {
            searchField_->Append("city");
            searchField_->Append("id_range");
            searchField_->SetSelection(0);
            searchField_->Enable(true);
            searchV1_->Enable(true);
            searchBtn_->Enable(true);
        } else if (tab == 1) {
            searchField_->Append("name");
            searchField_->Append("address_id");
            searchField_->SetSelection(0);
            searchField_->Enable(true);
            searchV1_->Enable(true);
            searchBtn_->Enable(true);
        } else if (tab == 2) {
            searchField_->Append("full_name");
            searchField_->Append("birth_year");
            searchField_->Append("dept_id");
            searchField_->SetSelection(0);
            searchField_->Enable(true);
            searchV1_->Enable(true);
            searchBtn_->Enable(true);
        } else if (tab == 3) {
            searchField_->Append("name");
            searchField_->Append("city");
            searchField_->SetSelection(0);
            searchField_->Enable(true);
            searchV1_->Enable(true);
            searchBtn_->Enable(true);
        } else if (tab == 4) {
            searchField_->Append("name");
            searchField_->Append("default_supplier_id");
            searchField_->SetSelection(0);
            searchField_->Enable(true);
            searchV1_->Enable(true);
            searchBtn_->Enable(true);
        } else if (tab == 5) {
            searchField_->Append("date");
            searchField_->Append("supplier_id");
            searchField_->Append("product_id");
            searchField_->Append("dept_id");
            searchField_->SetSelection(0);
            searchField_->Enable(true);
            searchV1_->Enable(true);
            searchBtn_->Enable(true);
        } else {
            searchField_->Append("(no search)");
            searchField_->SetSelection(0);
            searchField_->Enable(false);
            searchV1_->Enable(false);
            searchBtn_->Enable(false);
        }

        ApplyFieldUI(tab);
    }

    void OnTabChanged(wxBookCtrlEvent& evt) {
        UpdateSearchUIForTab(evt.GetSelection());
        evt.Skip();
    }

    void OnSearchFieldChanged(wxCommandEvent&) {
        ApplyFieldUI(notebook_ ? notebook_->GetSelection() : 0);
    }
void HighlightRowsByIds(int tab, const std::vector<int>& ids) {
        if (tab < 0 || tab >= (int)panels_.size()) return;

        wxGrid* g = panels_[tab]->GetGrid();
        if (!g) return;

        g->ClearSelection();

        if (ids.empty()) {
            wxMessageBox("No matches", "Search", wxOK | wxICON_INFORMATION, this);
            return;
        }

        std::unordered_set<int> set(ids.begin(), ids.end());

        int firstRow = -1;
        for (int r = 0; r < g->GetNumberRows(); ++r) {
            if (RowAllEmpty(g, r)) continue;

            long v = 0;
            if (!g->GetCellValue(r, 0).ToLong(&v)) continue;

            if (set.count((int)v)) {
                g->SelectRow(r, true);
                if (firstRow < 0) firstRow = r;
            }
        }

        if (firstRow >= 0) {
            g->MakeCellVisible(firstRow, 0);
            g->SetGridCursor(firstRow, 0);
        }
    }

    void OnClearSearch(wxCommandEvent&) {
        int tab = notebook_->GetSelection();
        if (tab >= 0 && tab < (int)panels_.size()) {
            if (auto* g = panels_[tab]->GetGrid()) g->ClearSelection();
        }
        if (searchV1_) searchV1_->Clear();
        if (searchV2_) searchV2_->Clear();
        AppendLog("Search cleared.");
    }


    void OnSearch(wxCommandEvent&) {
        try {
            const int tab = notebook_->GetSelection();
            const wxString field = searchField_ ? searchField_->GetStringSelection() : "";
            const SearchKind kind = GetSearchKind(tab, field);

            if (kind == SearchKind::Disabled) {
                wxMessageBox("No search available for this tab/field.", "Search", wxOK | wxICON_INFORMATION, this);
                return;
            }

            Database db = BuildDbFromGridsOrThrow(); // LoadFromFiles -> BuildIndexes

            auto getText = [&](wxTextCtrl* t) -> wxString {
                wxString s = t ? t->GetValue() : "";
                s.Trim(true).Trim(false);
                return s;
            };

            std::vector<int> ids;

            if (tab == 0) { // Addresses
                if (field == "city") {
                    wxString v = getText(searchV1_);
                    if (v.IsEmpty()) { wxMessageBox("Enter city", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindAddressIdsByCity(v.ToStdString());
                } else if (field == "id_range") {
                    long a=0,b=0;
                    if (!getText(searchV1_).ToLong(&a) || !getText(searchV2_).ToLong(&b)) {
                        wxMessageBox("Enter two integers (from/to)", "Search", wxOK | wxICON_WARNING, this); return;
                    }
                    if (b < a) std::swap(a,b);
                    ids = db.FindAddressIdsByIdRange((int)a,(int)b);
                }
            } else if (tab == 1) { // Departments
                if (field == "name") {
                    wxString v = getText(searchV1_);
                    if (v.IsEmpty()) { wxMessageBox("Enter department name", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindDepartmentIdsByName(v.ToStdString());
                } else if (field == "address_id") {
                    long v=0;
                    if (!getText(searchV1_).ToLong(&v)) { wxMessageBox("Enter address_id (integer)", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindDepartmentIdsByAddressId((int)v);
                }
            } else if (tab == 2) { // Employees
                if (field == "full_name") {
                    wxString v = getText(searchV1_);
                    if (v.IsEmpty()) { wxMessageBox("Enter full name (as stored)", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindEmployeeIdsByFullName(v.ToStdString());
                } else if (field == "birth_year") {
                    long a=0,b=0;
                    if (!getText(searchV1_).ToLong(&a) || !getText(searchV2_).ToLong(&b)) {
                        wxMessageBox("Enter two integers (from/to)", "Search", wxOK | wxICON_WARNING, this); return;
                    }
                    if (b < a) std::swap(a,b);
                    ids = db.FindEmployeeIdsByBirthYearRange((int)a,(int)b);
                } else if (field == "dept_id") {
                    long v=0;
                    if (!getText(searchV1_).ToLong(&v)) { wxMessageBox("Enter dept_id (integer)", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindEmployeeIdsByDeptId((int)v);
                }
            } else if (tab == 3) { // Suppliers
                if (field == "name") {
                    wxString v = getText(searchV1_);
                    if (v.IsEmpty()) { wxMessageBox("Enter supplier name", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindSupplierIdsByName(v.ToStdString());
                } else if (field == "city") {
                    wxString v = getText(searchV1_);
                    if (v.IsEmpty()) { wxMessageBox("Enter city", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindSupplierIdsByCity(v.ToStdString());
                }
            } else if (tab == 4) { // Products
                if (field == "name") {
                    wxString v = getText(searchV1_);
                    if (v.IsEmpty()) { wxMessageBox("Enter product name", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindProductIdsByName(v.ToStdString());
                } else if (field == "default_supplier_id") {
                    long v=0;
                    if (!getText(searchV1_).ToLong(&v)) { wxMessageBox("Enter default_supplier_id (integer)", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindProductIdsByDefaultSupplierId((int)v);
                }
            } else if (tab == 5) { // Purchases
                if (field == "date") {
                    wxString a = getText(searchV1_);
                    wxString b = getText(searchV2_);
                    if (a.IsEmpty() || b.IsEmpty()) { wxMessageBox("Enter two dates (from/to)", "Search", wxOK | wxICON_WARNING, this); return; }
                    if (b < a) std::swap(a,b);
                    ids = db.FindPurchaseIdsByDateRange(a.ToStdString(), b.ToStdString());
                } else if (field == "supplier_id") {
                    long v=0;
                    if (!getText(searchV1_).ToLong(&v)) { wxMessageBox("Enter supplier_id (integer)", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindPurchaseIdsBySupplierId((int)v);
                } else if (field == "product_id") {
                    long v=0;
                    if (!getText(searchV1_).ToLong(&v)) { wxMessageBox("Enter product_id (integer)", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindPurchaseIdsByProductId((int)v);
                } else if (field == "dept_id") {
                    long v=0;
                    if (!getText(searchV1_).ToLong(&v)) { wxMessageBox("Enter dept_id (integer)", "Search", wxOK | wxICON_WARNING, this); return; }
                    ids = db.FindPurchaseIdsByDeptId((int)v);
                }
            }

            HighlightRowsByIds(tab, ids);
            AppendLog(wxString::Format("Search: %zu rows matched.", ids.size()));
        } catch (const std::exception& e) {
            AppendLog(wxString::Format("Search failed: %s", e.what()));
            wxMessageBox(e.what(), "Search failed", wxOK | wxICON_ERROR, this);
        }
    }


    wxNotebook* notebook_{nullptr};
    wxButton* validateBtn_{nullptr};
    wxButton* addRowBtn_{nullptr};
    wxButton* deleteRowBtn_{nullptr};
    wxButton* saveAllAsBtn_{nullptr};
    wxButton* reloadAllBtn_{nullptr};
    wxChoice* searchField_{nullptr};
    wxStaticText* searchL1_{nullptr};
    wxStaticText* searchL2_{nullptr};
    wxTextCtrl* searchV1_{nullptr};
    wxTextCtrl* searchV2_{nullptr};
    wxButton* searchBtn_{nullptr};
    wxButton* clearSearchBtn_{nullptr};
    wxTextCtrl* log_{nullptr};
    std::vector<CsvTablePanel*> panels_;

    bool validatedOk_ = false;

    int lastTab_ = -1;
    int lastRow_ = -1;
    int lastCol_ = -1;
    wxColour lastOldColor_;
};

class MyApp : public wxApp {
public:
    bool OnInit() override {
        auto* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
