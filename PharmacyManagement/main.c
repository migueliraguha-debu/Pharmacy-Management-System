
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <stdarg.h>

#define MAX_MEDICINES 1000
#define MAX_SUPPLIERS 500

#define ID_ADD 1
#define ID_VIEW 2
#define ID_SELL 3
#define ID_SUPPLIERS 4
#define ID_UPDATE 5
#define ID_CHECK 6
#define ID_STOCK_VALUE 7
#define ID_HISTORY 8
#define ID_REPORT 9
#define ID_REPORT_TEXT 801
#define ID_REPORT_EXPORT 802

#define ID_HISTORY_LIST 700
#define ID_HISTORY_ID 701
#define ID_HISTORY_DATE 702
#define ID_HISTORY_FILTER 703
#define ID_HISTORY_ALL 704

#define ID_REMOVE_MEDICINE 202
#define ID_SUPPLIER_LIST 601
#define ID_SUPPLIER_ID 602
#define ID_SUPPLIER_NAME 603
#define ID_SUPPLIER_PHONE 604
#define ID_SUPPLIER_MEDICINES 605
#define ID_SUPPLIER_ADD 606
#define ID_SUPPLIER_REMOVE 607
#define ID_SUPPLIER_CLEAR 608

#define MEDICINE_FILE "medicines_v2.dat"
#define OLD_MEDICINE_FILE "medicines.dat"
#define SALES_FILE "sales_v2.dat"
#define OLD_SALES_FILE "sales.dat"
#define SUPPLIER_FILE "suppliers.dat"
#define HISTORY_FILE "history.dat"

typedef struct
{
    char id[40];
    char name[100];
    char category[20];
    double unitPrice;
    int quantity;
    char batch[50];
    char manufactureDate[11];
    char expiryDate[11];
    int requiresPrescription;
} Medicine;

/* Exact layout of the Medicine structure in the user's previous program. */
typedef struct
{
    char id[40];
    char name[100];
    char category[20];
    double unitPrice;
    int quantity;
    char batch[50];
    char manufactureDate[11];
    char expiryDate[11];
} LegacyMedicine;

typedef struct
{
    char medicineID[40];
    char medicineName[100];
    char customerName[100];
    char prescriptionNumber[50];
    int quantity;
    double unitPrice;
    double total; /* Final amount paid after any discount. */
    char dateTime[30];
    double subtotal;
    double discountPercent;
    double discountAmount;
    char discountReason[40];
} Sale;

/* Previous sale-record layout, used to preserve existing sales.dat records. */
typedef struct
{
    char medicineID[40];
    char medicineName[100];
    char customerName[100];
    char prescriptionNumber[50];
    int quantity;
    double unitPrice;
    double total;
    char dateTime[30];
} LegacySale;

typedef struct
{
    char id[40];
    char name[100];
    char phone[30];
    char medicinesSupplied[200];
} Supplier;

/* One permanent record for every important pharmacy operation. */
typedef struct
{
    char dateTime[30];
    char username[100];
    char action[40];
    char recordID[40];
    char details[300];
} HistoryRecord;

Medicine medicines[MAX_MEDICINES];
int medicineCount = 0;
Supplier suppliers[MAX_SUPPLIERS];
int supplierCount = 0;

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AddMedicineProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ViewMedicinesProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK UpdateStockProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SellMedicineProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CheckStockProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ManageSuppliersProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TransactionHistoryProcedure(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GenerateReportProcedure(HWND, UINT, WPARAM, LPARAM);

void HandleCheckStock(HWND hwnd);
void HandleCalculateStockValue(HWND hwnd);

void LoadMedicines(void);
int MigrateSalesFile(void);
int SaveMedicineToFile(Medicine medicine);
int SaveUpdatedMedicineFile(int updatedIndex, Medicine updatedMedicine);
int IsBlank(const char *text);
int ValidDate(const char *date);
int DaysUntilExpiry(const char *expiryDate);
void HandleSaveMedicine(HWND hwnd);
void HandleUpdateStock(HWND hwnd);
void HandleSellMedicine(HWND hwnd);
void AddListColumn(HWND list, int index, char *heading, int width);
void FillMedicineTable(HWND list);
void LoadSuppliers(void);
int SaveSupplierRecords(const Supplier *records, int count);
void FillSupplierTable(HWND list);
void HandleRemoveSelectedMedicine(HWND hwnd, HWND list);
void HandleAddSupplier(HWND hwnd);
void HandleRemoveSupplier(HWND hwnd);
int LogHistoryEvent(const char *action, const char *recordID, const char *details);
void FillHistoryTable(HWND hwnd);
int BuildPharmacyReport(char **reportText);
int ExportPharmacyReport(const char *reportText);
int AppendReportText(char **buffer, size_t *capacity, size_t *length, const char *format, ...);

/* LOAD NEW FILE, OR SAFELY MIGRATE THE OLD FILE */
void LoadMedicines(void)
{
    FILE *file;
    medicineCount = 0;

    file = fopen(MEDICINE_FILE, "rb");
    if (file != NULL)
    {
        while (medicineCount < MAX_MEDICINES &&
               fread(&medicines[medicineCount], sizeof(Medicine), 1, file) == 1)
            medicineCount++;

        fclose(file);
        return;
    }

    /* The new file does not exist. Read the old format without modifying it. */
    file = fopen(OLD_MEDICINE_FILE, "rb");
    if (file == NULL)
        return;

    {
        LegacyMedicine oldMedicine;

        while (medicineCount < MAX_MEDICINES &&
               fread(&oldMedicine, sizeof(LegacyMedicine), 1, file) == 1)
        {
            memset(&medicines[medicineCount], 0, sizeof(Medicine));

            strcpy(medicines[medicineCount].id, oldMedicine.id);
            strcpy(medicines[medicineCount].name, oldMedicine.name);
            strcpy(medicines[medicineCount].category, oldMedicine.category);

            medicines[medicineCount].unitPrice = oldMedicine.unitPrice;
            medicines[medicineCount].quantity = oldMedicine.quantity;

            strcpy(medicines[medicineCount].batch, oldMedicine.batch);
            strcpy(medicines[medicineCount].manufactureDate, oldMedicine.manufactureDate);
            strcpy(medicines[medicineCount].expiryDate, oldMedicine.expiryDate);

            medicines[medicineCount].requiresPrescription = 0;

            medicineCount++;
        }
    }

    fclose(file);

    /* Save migrated records in a separate file. The original medicines.dat stays untouched. */
    if (medicineCount > 0)
    {
        FILE *newFile = fopen(MEDICINE_FILE, "wb");

        if (newFile != NULL)
        {
            if (fwrite(medicines, sizeof(Medicine), medicineCount, newFile) != (size_t)medicineCount)
                MessageBox(NULL,
                           "Your old medicine records were loaded, but the new data file could not be created. Keep medicines.dat safe.",
                           "Migration Warning", MB_OK | MB_ICONWARNING);

            fclose(newFile);
        }
        else
        {
            MessageBox(NULL,
                       "Your old medicine records were loaded, but the new data file could not be created. Keep medicines.dat safe.",
                       "Migration Warning", MB_OK | MB_ICONWARNING);
        }
    }
}

/* MIGRATE OLD SALES RECORDS WITHOUT DELETING THE ORIGINAL FILE */
int MigrateSalesFile(void)
{
    FILE *oldFile;
    FILE *newFile;
    LegacySale oldSale;
    Sale newSale;
    size_t recordsRead;

    /* If the new file already exists, it is the active sales file. */
    newFile = fopen(SALES_FILE, "rb");
    if (newFile != NULL)
    {
        fclose(newFile);
        return 1;
    }

    oldFile = fopen(OLD_SALES_FILE, "rb");
    if (oldFile == NULL)
        return 1; /* No previous sales to migrate. */

    newFile = fopen("sales_v2.tmp", "wb");
    if (newFile == NULL)
    {
        fclose(oldFile);
        return 0;
    }

    while ((recordsRead = fread(&oldSale, sizeof(LegacySale), 1, oldFile)) == 1)
    {
        memset(&newSale, 0, sizeof(newSale));

        strcpy(newSale.medicineID, oldSale.medicineID);
        strcpy(newSale.medicineName, oldSale.medicineName);
        strcpy(newSale.customerName, oldSale.customerName);
        strcpy(newSale.prescriptionNumber, oldSale.prescriptionNumber);

        newSale.quantity = oldSale.quantity;
        newSale.unitPrice = oldSale.unitPrice;
        newSale.total = oldSale.total;
        strcpy(newSale.dateTime, oldSale.dateTime);

        /* Old sales had no discount, so their subtotal equals their total. */
        newSale.subtotal = oldSale.total;
        newSale.discountPercent = 0.0;
        newSale.discountAmount = 0.0;
        newSale.discountReason[0] = '\0';

        if (fwrite(&newSale, sizeof(Sale), 1, newFile) != 1)
        {
            fclose(oldFile);
            fclose(newFile);
            remove("sales_v2.tmp");
            return 0;
        }
    }

    if (ferror(oldFile))
    {
        fclose(oldFile);
        fclose(newFile);
        remove("sales_v2.tmp");
        return 0;
    }

    fclose(oldFile);

    if (fclose(newFile) != 0)
    {
        remove("sales_v2.tmp");
        return 0;
    }

    if (!MoveFileExA("sales_v2.tmp", SALES_FILE,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        remove("sales_v2.tmp");
        return 0;
    }

    return 1;
}

int IsBlank(const char *text)
{
    while (*text)
    {
        if (!isspace((unsigned char)*text))
            return 0;

        text++;
    }

    return 1;
}

int ValidDate(const char *date)
{
    int year, month, day;
    int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};

    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-')
        return 0;

    for (int i = 0; i < 10; i++)
        if (i != 4 && i != 7 && !isdigit((unsigned char)date[i]))
            return 0;

    if (sscanf(date, "%4d-%2d-%2d", &year, &month, &day) != 3)
        return 0;

    if (year < 1 || month < 1 || month > 12)
        return 0;

    if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))
        daysInMonth[1] = 29;

    return day >= 1 && day <= daysInMonth[month - 1];
}

int DaysUntilExpiry(const char *expiryDate)
{
    int year, month, day;
    SYSTEMTIME expiryTime, todayTime;
    FILETIME expiryFileTime, todayFileTime;
    ULARGE_INTEGER expiryValue, todayValue;
    LONGLONG difference;

    if (!ValidDate(expiryDate))
        return -999;

    sscanf(expiryDate, "%4d-%2d-%2d", &year, &month, &day);

    GetLocalTime(&todayTime);

    memset(&expiryTime, 0, sizeof(expiryTime));

    expiryTime.wYear = (WORD)year;
    expiryTime.wMonth = (WORD)month;
    expiryTime.wDay = (WORD)day;

    todayTime.wHour = 0;
    todayTime.wMinute = 0;
    todayTime.wSecond = 0;
    todayTime.wMilliseconds = 0;

    if (!SystemTimeToFileTime(&expiryTime, &expiryFileTime) ||
        !SystemTimeToFileTime(&todayTime, &todayFileTime))
        return -999;

    expiryValue.LowPart = expiryFileTime.dwLowDateTime;
    expiryValue.HighPart = expiryFileTime.dwHighDateTime;

    todayValue.LowPart = todayFileTime.dwLowDateTime;
    todayValue.HighPart = todayFileTime.dwHighDateTime;

    difference = (LONGLONG)expiryValue.QuadPart - (LONGLONG)todayValue.QuadPart;

    return (int)(difference / 864000000000LL);
}

int SaveMedicineToFile(Medicine medicine)
{
    FILE *file;

    if (medicineCount >= MAX_MEDICINES)
        return 0;

    file = fopen(MEDICINE_FILE, "ab");

    if (file == NULL)
        return 0;

    if (fwrite(&medicine, sizeof(Medicine), 1, file) != 1)
    {
        fclose(file);
        return 0;
    }

    fclose(file);

    medicines[medicineCount++] = medicine;

    return 1;
}

int SaveUpdatedMedicineFile(int updatedIndex, Medicine updatedMedicine)
{
    FILE *file = fopen("medicines_v2.tmp", "wb");
    int success = 1;

    if (file == NULL)
        return 0;

    for (int i = 0; i < medicineCount; i++)
    {
        Medicine record = (i == updatedIndex) ? updatedMedicine : medicines[i];

        if (fwrite(&record, sizeof(Medicine), 1, file) != 1)
        {
            success = 0;
            break;
        }
    }

    if (fclose(file) != 0)
        success = 0;

    if (!success)
    {
        remove("medicines_v2.tmp");
        return 0;
    }

    if (!MoveFileExA("medicines_v2.tmp", MEDICINE_FILE,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        remove("medicines_v2.tmp");
        return 0;
    }

    return 1;
}

/* ADD MEDICINE */
void HandleSaveMedicine(HWND hwnd)
{
    Medicine medicine = {0};

    char priceText[50], quantityText[30], today[11];
    char *endPrice, *endQuantity;

    long quantity;

    HWND category = GetDlgItem(hwnd, 103);
    HWND prescription = GetDlgItem(hwnd, 109);

    int selected, prescriptionSelected;

    GetDlgItemText(hwnd, 101, medicine.id, sizeof(medicine.id));
    GetDlgItemText(hwnd, 102, medicine.name, sizeof(medicine.name));
    GetDlgItemText(hwnd, 104, priceText, sizeof(priceText));
    GetDlgItemText(hwnd, 105, quantityText, sizeof(quantityText));
    GetDlgItemText(hwnd, 106, medicine.batch, sizeof(medicine.batch));
    GetDlgItemText(hwnd, 107, medicine.manufactureDate, sizeof(medicine.manufactureDate));
    GetDlgItemText(hwnd, 108, medicine.expiryDate, sizeof(medicine.expiryDate));

    if (IsBlank(medicine.id) || IsBlank(medicine.name) ||
        IsBlank(priceText) || IsBlank(quantityText) ||
        IsBlank(medicine.batch) || IsBlank(medicine.manufactureDate) ||
        IsBlank(medicine.expiryDate))
    {
        MessageBox(hwnd, "Please fill in all the fields.",
                   "Missing Information", MB_OK | MB_ICONWARNING);
        return;
    }

    selected = (int)SendMessage(category, CB_GETCURSEL, 0, 0);

    if (selected == CB_ERR)
    {
        MessageBox(hwnd, "Please select a medicine category.",
                   "Missing Category", MB_OK | MB_ICONWARNING);
        return;
    }

    SendMessage(category, CB_GETLBTEXT, selected, (LPARAM)medicine.category);

    prescriptionSelected = (int)SendMessage(prescription, CB_GETCURSEL, 0, 0);

    if (prescriptionSelected == CB_ERR)
    {
        MessageBox(hwnd, "Please choose Yes or No for prescription requirement.",
                   "Missing Information", MB_OK | MB_ICONWARNING);
        return;
    }

    medicine.requiresPrescription = (prescriptionSelected == 0) ? 1 : 0;

    for (int i = 0; i < medicineCount; i++)
    {
        if (strcmp(medicines[i].id, medicine.id) == 0)
        {
            MessageBox(hwnd, "This medicine ID already exists.",
                       "Duplicate ID", MB_OK | MB_ICONWARNING);
            return;
        }
    }

    errno = 0;

    medicine.unitPrice = strtod(priceText, &endPrice);

    while (isspace((unsigned char)*endPrice))
        endPrice++;

    if (errno == ERANGE || endPrice == priceText || *endPrice != '\0' ||
        !isfinite(medicine.unitPrice) || medicine.unitPrice <= 0)
    {
        MessageBox(hwnd, "Unit price must be a valid number greater than zero.",
                   "Invalid Price", MB_OK | MB_ICONWARNING);
        return;
    }

    errno = 0;

    quantity = strtol(quantityText, &endQuantity, 10);

    while (isspace((unsigned char)*endQuantity))
        endQuantity++;

    if (errno == ERANGE || endQuantity == quantityText || *endQuantity != '\0' ||
        quantity <= 0 || quantity > INT_MAX)
    {
        MessageBox(hwnd, "Quantity must be a whole number greater than zero.",
                   "Invalid Quantity", MB_OK | MB_ICONWARNING);
        return;
    }

    medicine.quantity = (int)quantity;

    if (!ValidDate(medicine.manufactureDate) || !ValidDate(medicine.expiryDate))
    {
        MessageBox(hwnd, "Please enter valid dates in YYYY-MM-DD format.",
                   "Invalid Date", MB_OK | MB_ICONWARNING);
        return;
    }

    {
        SYSTEMTIME currentTime;

        GetLocalTime(&currentTime);

        sprintf(today, "%04d-%02d-%02d",
                currentTime.wYear, currentTime.wMonth, currentTime.wDay);
    }

    if (strcmp(medicine.expiryDate, medicine.manufactureDate) <= 0)
    {
        MessageBox(hwnd, "Expiry date must be later than manufacture date.",
                   "Invalid Expiry Date", MB_OK | MB_ICONWARNING);
        return;
    }

    if (strcmp(medicine.expiryDate, today) <= 0)
    {
        MessageBox(hwnd, "Expiry date must be later than today.",
                   "Expired Medicine", MB_OK | MB_ICONWARNING);
        return;
    }

    if (!SaveMedicineToFile(medicine))
    {
        MessageBox(hwnd, "Could not save medicine. Check file access or storage capacity.",
                   "Save Error", MB_OK | MB_ICONERROR);
        return;
    }

    {
        char details[300];
        snprintf(details, sizeof(details), "Name: %s; Category: %s; Quantity added: %d; Unit price: %.2f RWF",
                medicine.name, medicine.category, medicine.quantity, medicine.unitPrice);
        if (!LogHistoryEvent("ADD MEDICINE", medicine.id, details))
            MessageBox(hwnd, "Medicine was added, but its history entry could not be saved.",
                       "History Warning", MB_OK | MB_ICONWARNING);
    }

    MessageBox(hwnd, "Medicine added successfully!",
               "Success", MB_OK | MB_ICONINFORMATION);

    SetDlgItemText(hwnd, 101, "");
    SetDlgItemText(hwnd, 102, "");
    SetDlgItemText(hwnd, 104, "");
    SetDlgItemText(hwnd, 105, "");
    SetDlgItemText(hwnd, 106, "");
    SetDlgItemText(hwnd, 107, "YYYY-MM-DD");
    SetDlgItemText(hwnd, 108, "YYYY-MM-DD");

    SendMessage(category, CB_SETCURSEL, (WPARAM)-1, 0);
    SendMessage(prescription, CB_SETCURSEL, (WPARAM)-1, 0);
}

/* UPDATE STOCK */
void HandleUpdateStock(HWND hwnd)
{
    char id[40], quantityText[30], newBatch[50], newExpiry[11], today[11];
    char *endQuantity;

    long additionalQuantity;
    int foundIndex = -1;

    Medicine updatedMedicine;
    char confirmation[300];

    GetDlgItemText(hwnd, 301, id, sizeof(id));
    GetDlgItemText(hwnd, 302, quantityText, sizeof(quantityText));
    GetDlgItemText(hwnd, 303, newBatch, sizeof(newBatch));
    GetDlgItemText(hwnd, 304, newExpiry, sizeof(newExpiry));

    if (IsBlank(id) || IsBlank(quantityText) ||
        IsBlank(newBatch) || IsBlank(newExpiry))
    {
        MessageBox(hwnd, "Please fill in all the fields.",
                   "Missing Information", MB_OK | MB_ICONWARNING);
        return;
    }

    for (int i = 0; i < medicineCount; i++)
        if (strcmp(medicines[i].id, id) == 0)
        {
            foundIndex = i;
            break;
        }

    if (foundIndex == -1)
    {
        MessageBox(hwnd, "No medicine was found with this ID.",
                   "Medicine Not Found", MB_OK | MB_ICONWARNING);
        return;
    }

    errno = 0;

    additionalQuantity = strtol(quantityText, &endQuantity, 10);

    while (isspace((unsigned char)*endQuantity))
        endQuantity++;

    if (errno == ERANGE || endQuantity == quantityText || *endQuantity != '\0' ||
        additionalQuantity <= 0 || additionalQuantity > INT_MAX)
    {
        MessageBox(hwnd, "Additional quantity must be a whole number greater than zero.",
                   "Invalid Quantity", MB_OK | MB_ICONWARNING);
        return;
    }

    if (additionalQuantity > INT_MAX - medicines[foundIndex].quantity)
    {
        MessageBox(hwnd, "The updated quantity is too large.",
                   "Quantity Limit", MB_OK | MB_ICONWARNING);
        return;
    }

    if (!ValidDate(newExpiry))
    {
        MessageBox(hwnd, "Enter a valid expiry date in YYYY-MM-DD format.",
                   "Invalid Date", MB_OK | MB_ICONWARNING);
        return;
    }

    {
        SYSTEMTIME currentTime;

        GetLocalTime(&currentTime);

        sprintf(today, "%04d-%02d-%02d",
                currentTime.wYear, currentTime.wMonth, currentTime.wDay);
    }

    if (strcmp(newExpiry, today) < 0 ||
        strcmp(newExpiry, medicines[foundIndex].manufactureDate) <= 0)
    {
        MessageBox(hwnd,
                   "Expiry date must not be in the past and must be later than manufacture date.",
                   "Invalid Expiry Date", MB_OK | MB_ICONWARNING);
        return;
    }

    updatedMedicine = medicines[foundIndex];

    updatedMedicine.quantity += (int)additionalQuantity;

    strcpy(updatedMedicine.batch, newBatch);
    strcpy(updatedMedicine.expiryDate, newExpiry);

    sprintf(confirmation,
            "Medicine: %s\nCurrent quantity: %d\nAdditional quantity: %ld\nNew quantity: %d\n\nSave this update?",
            updatedMedicine.name, medicines[foundIndex].quantity,
            additionalQuantity, updatedMedicine.quantity);

    if (MessageBox(hwnd, confirmation, "Confirm Stock Update",
                   MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    if (!SaveUpdatedMedicineFile(foundIndex, updatedMedicine))
    {
        MessageBox(hwnd,
                   "Could not update the medicine file. Your previous records have been kept.",
                   "Update Error", MB_OK | MB_ICONERROR);
        return;
    }

    medicines[foundIndex] = updatedMedicine;

    {
        char details[300];
        snprintf(details, sizeof(details), "Name: %s; Quantity added: %ld; New quantity: %d; New batch: %s; New expiry: %s",
                updatedMedicine.name, additionalQuantity, updatedMedicine.quantity,
                updatedMedicine.batch, updatedMedicine.expiryDate);
        if (!LogHistoryEvent("UPDATE STOCK", updatedMedicine.id, details))
            MessageBox(hwnd, "Stock was updated, but its history entry could not be saved.",
                       "History Warning", MB_OK | MB_ICONWARNING);
    }

    MessageBox(hwnd, "Stock updated successfully!",
               "Success", MB_OK | MB_ICONINFORMATION);

    SetDlgItemText(hwnd, 301, "");
    SetDlgItemText(hwnd, 302, "");
    SetDlgItemText(hwnd, 303, "");
    SetDlgItemText(hwnd, 304, "YYYY-MM-DD");
}

/* SELL MEDICINE WITH OPTIONAL DISCOUNT */
void HandleSellMedicine(HWND hwnd)
{
    char id[40], customer[100], quantityText[30], prescriptionNumber[50];
    char discountText[30], discountReason[40];
    char receipt[1400], dateTime[30];
    char *endQuantity, *endDiscount;
    long quantity;
    int foundIndex = -1;
    int reasonSelection;
    int hasDiscount = 0;
    Medicine updatedMedicine;
    Sale sale;
    SYSTEMTIME now;
    FILE *salesFile;
    double subtotal;
    double discountPercent = 0.0;
    double discountAmount = 0.0;
    double finalTotal;

    GetDlgItemText(hwnd, 401, id, sizeof(id));
    GetDlgItemText(hwnd, 402, customer, sizeof(customer));
    GetDlgItemText(hwnd, 403, quantityText, sizeof(quantityText));
    GetDlgItemText(hwnd, 404, prescriptionNumber, sizeof(prescriptionNumber));
    GetDlgItemText(hwnd, 407, discountText, sizeof(discountText));

    reasonSelection = (int)SendDlgItemMessage(hwnd, 408, CB_GETCURSEL, 0, 0);
    discountReason[0] = '\0';

    if (reasonSelection != CB_ERR)
        SendDlgItemMessage(hwnd, 408, CB_GETLBTEXT,
                           reasonSelection, (LPARAM)discountReason);

    if (IsBlank(id) || IsBlank(customer) || IsBlank(quantityText))
    {
        MessageBox(hwnd, "Please enter the medicine ID, customer name and quantity.",
                   "Missing Information", MB_OK | MB_ICONWARNING);
        return;
    }

    for (int i = 0; i < medicineCount; i++)
        if (strcmp(medicines[i].id, id) == 0)
        {
            foundIndex = i;
            break;
        }

    if (foundIndex == -1)
    {
        MessageBox(hwnd, "No medicine was found with this ID.",
                   "Medicine Not Found", MB_OK | MB_ICONWARNING);
        return;
    }

    if (DaysUntilExpiry(medicines[foundIndex].expiryDate) < 0)
    {
        MessageBox(hwnd, "This medicine has expired and cannot be sold.",
                   "Expired Medicine", MB_OK | MB_ICONERROR);
        return;
    }

    errno = 0;
    quantity = strtol(quantityText, &endQuantity, 10);

    while (isspace((unsigned char)*endQuantity))
        endQuantity++;

    if (errno == ERANGE || endQuantity == quantityText || *endQuantity != '\0' ||
        quantity <= 0 || quantity > INT_MAX)
    {
        MessageBox(hwnd, "Sale quantity must be a whole number greater than zero.",
                   "Invalid Quantity", MB_OK | MB_ICONWARNING);
        return;
    }

    if (quantity > medicines[foundIndex].quantity)
    {
        MessageBox(hwnd, "There is not enough stock for this sale.",
                   "Insufficient Stock", MB_OK | MB_ICONWARNING);
        return;
    }

    if (medicines[foundIndex].requiresPrescription && IsBlank(prescriptionNumber))
    {
        MessageBox(hwnd, "This medicine requires a prescription. Enter the prescription number.",
                   "Prescription Required", MB_OK | MB_ICONWARNING);
        return;
    }

    /* A discount is optional. If either discount field is used, both are required. */
    if (!IsBlank(discountText) || reasonSelection != CB_ERR)
    {
        if (IsBlank(discountText) || reasonSelection == CB_ERR)
        {
            MessageBox(hwnd,
                       "To apply a discount, enter a percentage and select a reason.",
                       "Incomplete Discount", MB_OK | MB_ICONWARNING);
            return;
        }

        errno = 0;
        discountPercent = strtod(discountText, &endDiscount);

        while (isspace((unsigned char)*endDiscount))
            endDiscount++;

        if (errno == ERANGE || endDiscount == discountText || *endDiscount != '\0' ||
            !isfinite(discountPercent) ||
            discountPercent < 5.0 || discountPercent > 15.0)
        {
            MessageBox(hwnd,
                       "Discount must be a valid number from 5% to 15%.",
                       "Invalid Discount", MB_OK | MB_ICONWARNING);
            return;
        }

        hasDiscount = 1;
    }

    subtotal = medicines[foundIndex].unitPrice * (double)quantity;

    if (!isfinite(subtotal) || subtotal <= 0.0)
    {
        MessageBox(hwnd, "The sale subtotal is invalid.",
                   "Calculation Error", MB_OK | MB_ICONERROR);
        return;
    }

    discountAmount = subtotal * discountPercent / 100.0;
    finalTotal = subtotal - discountAmount;

    if (!isfinite(discountAmount) || !isfinite(finalTotal) || finalTotal <= 0.0)
    {
        MessageBox(hwnd, "The discounted sale total is invalid.",
                   "Calculation Error", MB_OK | MB_ICONERROR);
        return;
    }

    memset(&sale, 0, sizeof(sale));

    strcpy(sale.medicineID, medicines[foundIndex].id);
    strcpy(sale.medicineName, medicines[foundIndex].name);
    strcpy(sale.customerName, customer);

    if (medicines[foundIndex].requiresPrescription)
        strcpy(sale.prescriptionNumber, prescriptionNumber);

    sale.quantity = (int)quantity;
    sale.unitPrice = medicines[foundIndex].unitPrice;
    sale.subtotal = subtotal;
    sale.discountPercent = discountPercent;
    sale.discountAmount = discountAmount;
    sale.total = finalTotal;

    if (hasDiscount)
        strcpy(sale.discountReason, discountReason);

    GetLocalTime(&now);
    sprintf(dateTime, "%04d-%02d-%02d %02d:%02d:%02d",
            now.wYear, now.wMonth, now.wDay,
            now.wHour, now.wMinute, now.wSecond);
    strcpy(sale.dateTime, dateTime);

    if (hasDiscount)
    {
        sprintf(receipt,
                "Medicine: %s\nMedicine ID: %s\nCustomer: %s\n"
                "Quantity: %ld\nUnit price: %.2f RWF\n"
                "Subtotal: %.2f RWF\nDiscount: %.2f%% (%s)\n"
                "Discount amount: %.2f RWF\nFINAL TOTAL: %.2f RWF\n"
                "%s%s\nDate: %s\n\nConfirm this sale?",
                sale.medicineName, sale.medicineID, sale.customerName,
                quantity, sale.unitPrice, sale.subtotal,
                sale.discountPercent, sale.discountReason,
                sale.discountAmount, sale.total,
                medicines[foundIndex].requiresPrescription ? "Prescription: " : "",
                medicines[foundIndex].requiresPrescription ? sale.prescriptionNumber : "",
                sale.dateTime);
    }
    else
    {
        sprintf(receipt,
                "Medicine: %s\nMedicine ID: %s\nCustomer: %s\n"
                "Quantity: %ld\nUnit price: %.2f RWF\n"
                "Subtotal: %.2f RWF\nDiscount: None\nFINAL TOTAL: %.2f RWF\n"
                "%s%s\nDate: %s\n\nConfirm this sale?",
                sale.medicineName, sale.medicineID, sale.customerName,
                quantity, sale.unitPrice, sale.subtotal, sale.total,
                medicines[foundIndex].requiresPrescription ? "Prescription: " : "",
                medicines[foundIndex].requiresPrescription ? sale.prescriptionNumber : "",
                sale.dateTime);
    }

    if (MessageBox(hwnd, receipt, "Confirm Sale",
                   MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    /* Save the sale record first. Do not deduct stock if this fails. */
    salesFile = fopen(SALES_FILE, "ab");

    if (salesFile == NULL)
    {
        MessageBox(hwnd, "Could not open the sales file. No stock was deducted.",
                   "Sale Save Error", MB_OK | MB_ICONERROR);
        return;
    }

    {
        int saleWriteOK = (fwrite(&sale, sizeof(Sale), 1, salesFile) == 1);
        int saleCloseOK = (fclose(salesFile) == 0);

        if (!saleWriteOK || !saleCloseOK)
        {
            MessageBox(hwnd, "Could not save the sale record. Check the sales file.",
                       "Sale Save Error", MB_OK | MB_ICONERROR);
            return;
        }
    }

    updatedMedicine = medicines[foundIndex];
    updatedMedicine.quantity -= (int)quantity;

    if (!SaveUpdatedMedicineFile(foundIndex, updatedMedicine))
    {
        MessageBox(hwnd,
                   "The sale record was saved, but stock could not be updated. Do not repeat the sale. Please contact the project administrator.",
                   "Stock Update Error", MB_OK | MB_ICONERROR);
        return;
    }

    medicines[foundIndex] = updatedMedicine;

    {
        char details[300];
        if (hasDiscount)
            snprintf(details, sizeof(details), "Medicine: %s; Customer: %s; Quantity: %ld; Paid: %.2f RWF; Discount: %.2f%% (%s), amount %.2f RWF",
                    sale.medicineName, sale.customerName, quantity, sale.total,
                    sale.discountPercent, sale.discountReason, sale.discountAmount);
        else
            snprintf(details, sizeof(details), "Medicine: %s; Customer: %s; Quantity: %ld; Paid: %.2f RWF; No discount",
                    sale.medicineName, sale.customerName, quantity, sale.total);

        if (!LogHistoryEvent(hasDiscount ? "SALE + DISCOUNT" : "SELL MEDICINE",
                             sale.medicineID, details))
            MessageBox(hwnd, "Sale was completed, but its history entry could not be saved.",
                       "History Warning", MB_OK | MB_ICONWARNING);
    }

    if (hasDiscount)
    {
        sprintf(receipt,
                "SALE COMPLETED\n\nMedicine: %s\nMedicine ID: %s\n"
                "Customer: %s\nQuantity sold: %ld\nUnit price: %.2f RWF\n"
                "Subtotal: %.2f RWF\nDiscount: %.2f%% (%s)\n"
                "Discount amount: %.2f RWF\nFINAL TOTAL: %.2f RWF\n"
                "Remaining stock: %d\n%s%s\nDate: %s",
                sale.medicineName, sale.medicineID, sale.customerName,
                quantity, sale.unitPrice, sale.subtotal,
                sale.discountPercent, sale.discountReason,
                sale.discountAmount, sale.total, updatedMedicine.quantity,
                medicines[foundIndex].requiresPrescription ? "Prescription: " : "",
                medicines[foundIndex].requiresPrescription ? sale.prescriptionNumber : "",
                sale.dateTime);
    }
    else
    {
        sprintf(receipt,
                "SALE COMPLETED\n\nMedicine: %s\nMedicine ID: %s\n"
                "Customer: %s\nQuantity sold: %ld\nUnit price: %.2f RWF\n"
                "Subtotal: %.2f RWF\nDiscount: None\nFINAL TOTAL: %.2f RWF\n"
                "Remaining stock: %d\n%s%s\nDate: %s",
                sale.medicineName, sale.medicineID, sale.customerName,
                quantity, sale.unitPrice, sale.subtotal, sale.total,
                updatedMedicine.quantity,
                medicines[foundIndex].requiresPrescription ? "Prescription: " : "",
                medicines[foundIndex].requiresPrescription ? sale.prescriptionNumber : "",
                sale.dateTime);
    }

    MessageBox(hwnd, receipt, "Sales Receipt",
               MB_OK | MB_ICONINFORMATION);

    SetDlgItemText(hwnd, 401, "");
    SetDlgItemText(hwnd, 402, "");
    SetDlgItemText(hwnd, 403, "");
    SetDlgItemText(hwnd, 404, "");
    SetDlgItemText(hwnd, 407, "");
    SendDlgItemMessage(hwnd, 408, CB_SETCURSEL, (WPARAM)-1, 0);
}

/* CHECK STOCK AND EXPIRY */
void HandleCheckStock(HWND hwnd)
{
    char id[40];
    char result[1000];
    char warning[300] = "";

    int foundIndex = -1;
    int days;

    GetDlgItemText(hwnd, 501, id, sizeof(id));

    if (IsBlank(id))
    {
        MessageBox(hwnd, "Please enter a medicine ID.",
                   "Missing Information", MB_OK | MB_ICONWARNING);
        return;
    }

    for (int i = 0; i < medicineCount; i++)
    {
        if (strcmp(medicines[i].id, id) == 0)
        {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex == -1)
    {
        SetDlgItemText(hwnd, 504,
                       "Medicine not found. Check the ID and try again.");
        return;
    }

    days = DaysUntilExpiry(medicines[foundIndex].expiryDate);

    if (days < 0)
        strcat(warning, "STATUS: EXPIRED - DO NOT SELL\n");

    else if (days <= 30)
        strcat(warning, "WARNING: Medicine expires within 30 days.\n");

    if (medicines[foundIndex].quantity < 10)
        strcat(warning, "WARNING: Low stock - quantity is below 10.\n");

    if (warning[0] == '\0')
        strcpy(warning, "STATUS: Stock and expiry are satisfactory.\n");

    sprintf(result,
            "MEDICINE DETAILS\n\n"
            "Medicine ID: %s\n"
            "Name: %s\n"
            "Category: %s\n"
            "Quantity available: %d\n"
            "Unit price: %.2f RWF\n"
            "Batch number: %s\n"
            "Expiry date: %s\n\n"
            "%s",
            medicines[foundIndex].id,
            medicines[foundIndex].name,
            medicines[foundIndex].category,
            medicines[foundIndex].quantity,
            medicines[foundIndex].unitPrice,
            medicines[foundIndex].batch,
            medicines[foundIndex].expiryDate,
            warning);

    SetDlgItemText(hwnd, 504, result);
}

/* CALCULATE STOCK VALUE */
void HandleCalculateStockValue(HWND hwnd)
{
    double currentStockValue = 0.0;
    double expiredStockLoss = 0.0;
    long long currentUnits = 0;
    long long expiredUnits = 0;
    int currentMedicineCount = 0;
    int expiredMedicineCount = 0;
    char result[1000];

    if (medicineCount == 0)
    {
        MessageBox(hwnd,
                   "There are no registered medicines to calculate.",
                   "Stock Value", MB_OK | MB_ICONINFORMATION);
        return;
    }

    for (int i = 0; i < medicineCount; i++)
    {
        double itemValue;
        int daysUntilExpiry;

        itemValue = medicines[i].unitPrice * (double)medicines[i].quantity;

        if (!isfinite(itemValue) || itemValue < 0.0)
        {
            MessageBox(hwnd,
                       "A medicine has an invalid stock value. Please check its price and quantity.",
                       "Calculation Error", MB_OK | MB_ICONERROR);
            return;
        }

        daysUntilExpiry = DaysUntilExpiry(medicines[i].expiryDate);

        if (daysUntilExpiry == -999)
        {
            MessageBox(hwnd,
                       "A medicine has an invalid expiry date. Stock value cannot be calculated safely.",
                       "Invalid Expiry Date", MB_OK | MB_ICONERROR);
            return;
        }

        if (daysUntilExpiry < 0)
        {
            expiredStockLoss += itemValue;
            expiredUnits += medicines[i].quantity;
            expiredMedicineCount++;
        }
        else
        {
            currentStockValue += itemValue;
            currentUnits += medicines[i].quantity;
            currentMedicineCount++;
        }

        if (!isfinite(currentStockValue) || !isfinite(expiredStockLoss))
        {
            MessageBox(hwnd,
                       "The stock value is too large to calculate.",
                       "Calculation Error", MB_OK | MB_ICONERROR);
            return;
        }
    }

    sprintf(result,
            "PHARMACY STOCK VALUATION\n\n"
            "CURRENT STOCK VALUE (not expired):\n"
            "%.2f RWF\n"
            "Medicines: %d\n"
            "Units in stock: %lld\n\n"
            "EXPIRED STOCK LOSS:\n"
            "%.2f RWF\n"
            "Medicines: %d\n"
            "Units expired: %lld\n\n"
            "Calculation: quantity x unit price.\n"
            "Medicines expiring today are counted as not expired.",
            currentStockValue, currentMedicineCount, currentUnits,
            expiredStockLoss, expiredMedicineCount, expiredUnits);

    MessageBox(hwnd, result, "Calculate Stock Value",
               MB_OK | MB_ICONINFORMATION);
}

/* TABLE HELPERS */
void AddListColumn(HWND list, int index, char *heading, int width)
{
    LVCOLUMN column = {0};

    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.pszText = heading;
    column.cx = width;
    column.iSubItem = index;

    ListView_InsertColumn(list, index, &column);
}

void FillMedicineTable(HWND list)
{
    ListView_DeleteAllItems(list);

    for (int i = 0; i < medicineCount; i++)
    {
        char quantity[20], price[30];

        LVITEM item = {0};

        sprintf(quantity, "%d", medicines[i].quantity);
        sprintf(price, "%.2f", medicines[i].unitPrice);

        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.pszText = medicines[i].id;

        ListView_InsertItem(list, &item);

        ListView_SetItemText(list, i, 1, medicines[i].name);
        ListView_SetItemText(list, i, 2, medicines[i].category);
        ListView_SetItemText(list, i, 3, quantity);
        ListView_SetItemText(list, i, 4, price);
        ListView_SetItemText(list, i, 5, medicines[i].batch);
        ListView_SetItemText(list, i, 6, medicines[i].expiryDate);
    }
}

/* REMOVE A MEDICINE WITHOUT DAMAGING THE EXISTING DATA FILE */
int SaveMedicinesWithoutIndex(int removedIndex)
{
    FILE *file = fopen("medicines_v2.tmp", "wb");
    int success = 1;

    if (file == NULL)
        return 0;

    for (int i = 0; i < medicineCount; i++)
    {
        if (i == removedIndex)
            continue;

        if (fwrite(&medicines[i], sizeof(Medicine), 1, file) != 1)
        {
            success = 0;
            break;
        }
    }

    if (fclose(file) != 0)
        success = 0;

    if (!success)
    {
        remove("medicines_v2.tmp");
        return 0;
    }

    if (!MoveFileExA("medicines_v2.tmp", MEDICINE_FILE,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        remove("medicines_v2.tmp");
        return 0;
    }

    return 1;
}

void HandleRemoveSelectedMedicine(HWND hwnd, HWND list)
{
    int selected = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    char message[500];

    if (selected < 0 || selected >= medicineCount)
    {
        MessageBox(hwnd, "Please select a medicine from the table first.",
                   "No Medicine Selected", MB_OK | MB_ICONWARNING);
        return;
    }

    if (DaysUntilExpiry(medicines[selected].expiryDate) < 0)
        sprintf(message,
                "This medicine is EXPIRED.\n\nMedicine: %s\nID: %s\nQuantity: %d\n\nRemove this expired medicine from the records?",
                medicines[selected].name, medicines[selected].id,
                medicines[selected].quantity);
    else
        sprintf(message,
                "Medicine: %s\nID: %s\nQuantity: %d\n\nAre you sure you want to remove this medicine?",
                medicines[selected].name, medicines[selected].id,
                medicines[selected].quantity);

    if (MessageBox(hwnd, message, "Confirm Medicine Removal",
                   MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        return;

    if (!SaveMedicinesWithoutIndex(selected))
    {
        MessageBox(hwnd,
                   "The medicine could not be removed from the data file. Your existing records have been kept.",
                   "Removal Error", MB_OK | MB_ICONERROR);
        return;
    }

    {
        char details[300];
        snprintf(details, sizeof(details), "Name: %s; Quantity at removal: %d; Expiry: %s",
                medicines[selected].name, medicines[selected].quantity,
                medicines[selected].expiryDate);
        if (!LogHistoryEvent("REMOVE MEDICINE", medicines[selected].id, details))
            MessageBox(hwnd, "Medicine was removed, but its history entry could not be saved.",
                       "History Warning", MB_OK | MB_ICONWARNING);
    }

    for (int i = selected; i < medicineCount - 1; i++)
        medicines[i] = medicines[i + 1];

    medicineCount--;
    ListView_DeleteAllItems(list);
    FillMedicineTable(list);

    MessageBox(hwnd, "Medicine removed successfully.",
               "Removal Complete", MB_OK | MB_ICONINFORMATION);
}

/* SUPPLIER FILE STORAGE */
void LoadSuppliers(void)
{
    FILE *file = fopen(SUPPLIER_FILE, "rb");
    supplierCount = 0;

    if (file == NULL)
        return; /* A first-time installation may not have supplier records yet. */

    while (supplierCount < MAX_SUPPLIERS &&
           fread(&suppliers[supplierCount], sizeof(Supplier), 1, file) == 1)
        supplierCount++;

    fclose(file);
}

int SaveSupplierRecords(const Supplier *records, int count)
{
    FILE *file = fopen("suppliers.tmp", "wb");
    int success = 1;

    if (file == NULL)
        return 0;

    if (count > 0 && fwrite(records, sizeof(Supplier), (size_t)count, file) != (size_t)count)
        success = 0;

    if (fclose(file) != 0)
        success = 0;

    if (!success)
    {
        remove("suppliers.tmp");
        return 0;
    }

    if (!MoveFileExA("suppliers.tmp", SUPPLIER_FILE,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        remove("suppliers.tmp");
        return 0;
    }

    return 1;
}

void FillSupplierTable(HWND list)
{
    ListView_DeleteAllItems(list);

    for (int i = 0; i < supplierCount; i++)
    {
        LVITEM item = {0};
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.pszText = suppliers[i].id;
        ListView_InsertItem(list, &item);
        ListView_SetItemText(list, i, 1, suppliers[i].name);
        ListView_SetItemText(list, i, 2, suppliers[i].phone);
        ListView_SetItemText(list, i, 3, suppliers[i].medicinesSupplied);
    }
}

void HandleAddSupplier(HWND hwnd)
{
    Supplier supplier = {0};

    GetDlgItemText(hwnd, ID_SUPPLIER_ID, supplier.id, sizeof(supplier.id));
    GetDlgItemText(hwnd, ID_SUPPLIER_NAME, supplier.name, sizeof(supplier.name));
    GetDlgItemText(hwnd, ID_SUPPLIER_PHONE, supplier.phone, sizeof(supplier.phone));
    GetDlgItemText(hwnd, ID_SUPPLIER_MEDICINES, supplier.medicinesSupplied,
                   sizeof(supplier.medicinesSupplied));

    if (IsBlank(supplier.id) || IsBlank(supplier.name) ||
        IsBlank(supplier.phone) || IsBlank(supplier.medicinesSupplied))
    {
        MessageBox(hwnd, "Please complete all supplier fields.",
                   "Missing Information", MB_OK | MB_ICONWARNING);
        return;
    }

    if (supplierCount >= MAX_SUPPLIERS)
    {
        MessageBox(hwnd, "The supplier record limit has been reached.",
                   "Storage Limit", MB_OK | MB_ICONWARNING);
        return;
    }

    for (int i = 0; i < supplierCount; i++)
    {
        if (strcmp(suppliers[i].id, supplier.id) == 0)
        {
            MessageBox(hwnd, "This supplier ID already exists. Use a unique ID.",
                       "Duplicate Supplier ID", MB_OK | MB_ICONWARNING);
            return;
        }
    }

    /* Write the proposed list first; update memory only after the file is safely replaced. */
    {
        Supplier proposed[MAX_SUPPLIERS];
        memcpy(proposed, suppliers, (size_t)supplierCount * sizeof(Supplier));
        proposed[supplierCount] = supplier;

        if (!SaveSupplierRecords(proposed, supplierCount + 1))
        {
            MessageBox(hwnd, "Could not save the supplier. Existing supplier records were kept.",
                       "Save Error", MB_OK | MB_ICONERROR);
            return;
        }
    }

    suppliers[supplierCount++] = supplier;
    {
        char details[300];
        snprintf(details, sizeof(details), "Supplier: %s; Phone: %s; Medicines supplied: %s",
                supplier.name, supplier.phone, supplier.medicinesSupplied);
        if (!LogHistoryEvent("ADD SUPPLIER", supplier.id, details))
            MessageBox(hwnd, "Supplier was added, but its history entry could not be saved.",
                       "History Warning", MB_OK | MB_ICONWARNING);
    }
    FillSupplierTable(GetDlgItem(hwnd, ID_SUPPLIER_LIST));

    SetDlgItemText(hwnd, ID_SUPPLIER_ID, "");
    SetDlgItemText(hwnd, ID_SUPPLIER_NAME, "");
    SetDlgItemText(hwnd, ID_SUPPLIER_PHONE, "");
    SetDlgItemText(hwnd, ID_SUPPLIER_MEDICINES, "");

    MessageBox(hwnd, "Supplier added successfully.",
               "Success", MB_OK | MB_ICONINFORMATION);
}

void HandleRemoveSupplier(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, ID_SUPPLIER_LIST);
    int selected = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    char message[350];

    if (selected < 0 || selected >= supplierCount)
    {
        MessageBox(hwnd, "Select a supplier from the table first.",
                   "No Supplier Selected", MB_OK | MB_ICONWARNING);
        return;
    }

    sprintf(message, "Supplier: %s\nSupplier ID: %s\n\nRemove this supplier?",
            suppliers[selected].name, suppliers[selected].id);

    if (MessageBox(hwnd, message, "Confirm Supplier Removal",
                   MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
        return;

    {
        Supplier proposed[MAX_SUPPLIERS];
        int newCount = supplierCount - 1;
        int destination = 0;

        for (int i = 0; i < supplierCount; i++)
            if (i != selected)
                proposed[destination++] = suppliers[i];

        if (!SaveSupplierRecords(proposed, newCount))
        {
            MessageBox(hwnd, "Could not remove the supplier from the data file. Existing records were kept.",
                       "Removal Error", MB_OK | MB_ICONERROR);
            return;
        }

        {
            char details[300];
            snprintf(details, sizeof(details), "Supplier removed: %s; Phone: %s; Medicines supplied: %s",
                    suppliers[selected].name, suppliers[selected].phone,
                    suppliers[selected].medicinesSupplied);
            if (!LogHistoryEvent("REMOVE SUPPLIER", suppliers[selected].id, details))
                MessageBox(hwnd, "Supplier was removed, but its history entry could not be saved.",
                           "History Warning", MB_OK | MB_ICONWARNING);
        }

        memcpy(suppliers, proposed, (size_t)newCount * sizeof(Supplier));
        supplierCount = newCount;
    }

    FillSupplierTable(list);
    MessageBox(hwnd, "Supplier removed successfully.",
               "Removal Complete", MB_OK | MB_ICONINFORMATION);
}

/* TRANSACTION HISTORY STORAGE AND DISPLAY */
int LogHistoryEvent(const char *action, const char *recordID, const char *details)
{
    FILE *file;
    HistoryRecord record;
    SYSTEMTIME now;
    DWORD usernameLength;

    memset(&record, 0, sizeof(record));
    GetLocalTime(&now);

    sprintf(record.dateTime, "%04u-%02u-%02u %02u:%02u:%02u",
            (unsigned)now.wYear, (unsigned)now.wMonth, (unsigned)now.wDay,
            (unsigned)now.wHour, (unsigned)now.wMinute, (unsigned)now.wSecond);

    usernameLength = GetEnvironmentVariableA("USERNAME", record.username,
                                              (DWORD)sizeof(record.username));
    if (usernameLength == 0 || usernameLength >= sizeof(record.username))
        strcpy(record.username, "Unknown");

    strncpy(record.action, action, sizeof(record.action) - 1);
    strncpy(record.recordID, recordID, sizeof(record.recordID) - 1);
    strncpy(record.details, details, sizeof(record.details) - 1);

    file = fopen(HISTORY_FILE, "ab");
    if (file == NULL)
        return 0;

    if (fwrite(&record, sizeof(record), 1, file) != 1)
    {
        fclose(file);
        return 0;
    }

    if (fclose(file) != 0)
        return 0;

    return 1;
}

void FillHistoryTable(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, ID_HISTORY_LIST);
    char idFilter[40], dateFilter[32];
    FILE *file;
    HistoryRecord record;
    int row = 0;

    GetDlgItemText(hwnd, ID_HISTORY_ID, idFilter, sizeof(idFilter));
    GetDlgItemText(hwnd, ID_HISTORY_DATE, dateFilter, sizeof(dateFilter));

    ListView_DeleteAllItems(list);

    file = fopen(HISTORY_FILE, "rb");
    if (file == NULL)
        return;

    /* Records are appended to the file, so they appear in recorded order. */
    while (fread(&record, sizeof(record), 1, file) == 1)
    {
        LVITEM item = {0};
        char dateOnly[11];

        strncpy(dateOnly, record.dateTime, 10);
        dateOnly[10] = '\0';

        if (!IsBlank(idFilter) && strstr(record.recordID, idFilter) == NULL)
            continue;

        if (!IsBlank(dateFilter) && strcmp(dateOnly, dateFilter) != 0)
            continue;

        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.pszText = record.dateTime;
        if (ListView_InsertItem(list, &item) >= 0)
        {
            ListView_SetItemText(list, row, 1, record.username);
            ListView_SetItemText(list, row, 2, record.action);
            ListView_SetItemText(list, row, 3, record.recordID);
            ListView_SetItemText(list, row, 4, record.details);
            row++;
        }
    }

    fclose(file);
}

LRESULT CALLBACK TransactionHistoryProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            HWND list;

            CreateWindow("STATIC", "TRANSACTION HISTORY",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         300, 12, 350, 28, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Medicine / Supplier ID:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         20, 48, 145, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         170, 48, 180, 25, hwnd, (HMENU)ID_HISTORY_ID, NULL, NULL);

            CreateWindow("STATIC", "Date (YYYY-MM-DD):",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         370, 48, 135, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         510, 48, 150, 25, hwnd, (HMENU)ID_HISTORY_DATE, NULL, NULL);

            CreateWindow("BUTTON", "Filter",
                         WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         680, 45, 90, 32, hwnd, (HMENU)ID_HISTORY_FILTER, NULL, NULL);
            CreateWindow("BUTTON", "Show All",
                         WS_VISIBLE | WS_CHILD,
                         780, 45, 100, 32, hwnd, (HMENU)ID_HISTORY_ALL, NULL, NULL);

            list = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
                                  WS_VISIBLE | WS_CHILD | LVS_REPORT |
                                  LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                  15, 90, 950, 480, hwnd, (HMENU)ID_HISTORY_LIST,
                                  GetModuleHandle(NULL), NULL);
            ListView_SetExtendedListViewStyle(list,
                                              LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            AddListColumn(list, 0, "Date / Time", 145);
            AddListColumn(list, 1, "User", 100);
            AddListColumn(list, 2, "Action", 135);
            AddListColumn(list, 3, "Medicine / Supplier ID", 145);
            AddListColumn(list, 4, "Details", 405);

            FillHistoryTable(hwnd);
            return 0;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_HISTORY_FILTER)
            {
                char dateFilter[32];
                GetDlgItemText(hwnd, ID_HISTORY_DATE, dateFilter, sizeof(dateFilter));
                if (!IsBlank(dateFilter) && !ValidDate(dateFilter))
                {
                    MessageBox(hwnd, "Enter the date in YYYY-MM-DD format, or leave it blank.",
                               "Invalid Date Filter", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                FillHistoryTable(hwnd);
            }
            else if (LOWORD(wParam) == ID_HISTORY_ALL)
            {
                SetDlgItemText(hwnd, ID_HISTORY_ID, "");
                SetDlgItemText(hwnd, ID_HISTORY_DATE, "");
                FillHistoryTable(hwnd);
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* GENERATE PHARMACY REPORT */
typedef struct
{
    char medicineID[40];
    char medicineName[100];
    long long quantitySold;
} BestSellerRecord;

int AppendReportText(char **buffer, size_t *capacity, size_t *length,
                     const char *format, ...)
{
    va_list args;
    va_list copyArgs;
    int needed;

    va_start(args, format);
    va_copy(copyArgs, args);
    needed = vsnprintf(NULL, 0, format, copyArgs);
    va_end(copyArgs);

    if (needed < 0)
    {
        va_end(args);
        return 0;
    }

    if (*length + (size_t)needed + 1 > *capacity)
    {
        size_t newCapacity = (*capacity == 0) ? 2048 : *capacity;
        char *newBuffer;

        while (newCapacity < *length + (size_t)needed + 1)
        {
            if (newCapacity > ((size_t)-1) / 2)
            {
                va_end(args);
                return 0;
            }
            newCapacity *= 2;
        }

        newBuffer = (char *)realloc(*buffer, newCapacity);
        if (newBuffer == NULL)
        {
            va_end(args);
            return 0;
        }

        *buffer = newBuffer;
        *capacity = newCapacity;
    }

    vsnprintf(*buffer + *length, *capacity - *length, format, args);
    *length += (size_t)needed;
    va_end(args);
    return 1;
}

int BuildPharmacyReport(char **reportText)
{
    double currentStockValue = 0.0;
    double expiredStockLoss = 0.0;
    double totalSalesRevenue = 0.0;
    long long totalUnits = 0;
    long long bestSellerQuantity = 0;
    int lowStockCount = 0;
    int expiringCount = 0;
    int invalidExpiryCount = 0;
    int salesToday = 0;
    int bestSellerIndex = -1;
    char today[11];
    SYSTEMTIME now;
    char *buffer = NULL;
    size_t capacity = 0, length = 0;
    FILE *salesFile;
    Sale sale;
    BestSellerRecord *bestSellers = NULL;
    size_t bestSellerCount = 0, bestSellerCapacity = 0;
    int i;

    *reportText = NULL;
    GetLocalTime(&now);
    snprintf(today, sizeof(today), "%04u-%02u-%02u",
             (unsigned)now.wYear, (unsigned)now.wMonth, (unsigned)now.wDay);

    for (i = 0; i < medicineCount; i++)
    {
        int days = DaysUntilExpiry(medicines[i].expiryDate);
        double value = medicines[i].unitPrice * (double)medicines[i].quantity;

        if (!isfinite(value) || value < 0.0)
        {
            free(buffer);
            return 0;
        }

        totalUnits += medicines[i].quantity;

        if (days == -999)
            invalidExpiryCount++;
        else if (days < 0)
            expiredStockLoss += value;
        else
            currentStockValue += value;

        if (medicines[i].quantity < 10)
            lowStockCount++;
        if (days >= 0 && days <= 30)
            expiringCount++;

        if (!isfinite(currentStockValue) || !isfinite(expiredStockLoss))
        {
            free(buffer);
            return 0;
        }
    }

    /* No sales file is a valid first-time state: revenue and sales count remain zero. */
    salesFile = fopen(SALES_FILE, "rb");
    if (salesFile != NULL)
    {
        while (fread(&sale, sizeof(Sale), 1, salesFile) == 1)
        {
            size_t j;
            int found = 0;

            if (isfinite(sale.total) && sale.total >= 0.0)
                totalSalesRevenue += sale.total;
            if (strncmp(sale.dateTime, today, 10) == 0)
                salesToday++;

            for (j = 0; j < bestSellerCount; j++)
            {
                if (strcmp(bestSellers[j].medicineID, sale.medicineID) == 0)
                {
                    bestSellers[j].quantitySold += sale.quantity;
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                if (bestSellerCount == bestSellerCapacity)
                {
                    size_t newCapacity = bestSellerCapacity == 0 ? 16 : bestSellerCapacity * 2;
                    BestSellerRecord *newRecords;
                    if (newCapacity < bestSellerCapacity ||
                        newCapacity > ((size_t)-1) / sizeof(BestSellerRecord))
                    {
                        fclose(salesFile);
                        free(bestSellers);
                        free(buffer);
                        return 0;
                    }
                    newRecords = (BestSellerRecord *)realloc(bestSellers,
                                      newCapacity * sizeof(BestSellerRecord));
                    if (newRecords == NULL)
                    {
                        fclose(salesFile);
                        free(bestSellers);
                        free(buffer);
                        return 0;
                    }
                    bestSellers = newRecords;
                    bestSellerCapacity = newCapacity;
                }

                memset(&bestSellers[bestSellerCount], 0, sizeof(BestSellerRecord));
                strncpy(bestSellers[bestSellerCount].medicineID, sale.medicineID,
                        sizeof(bestSellers[bestSellerCount].medicineID) - 1);
                strncpy(bestSellers[bestSellerCount].medicineName, sale.medicineName,
                        sizeof(bestSellers[bestSellerCount].medicineName) - 1);
                bestSellers[bestSellerCount].quantitySold = sale.quantity;
                bestSellerCount++;
            }
        }

        if (ferror(salesFile))
        {
            fclose(salesFile);
            free(bestSellers);
            free(buffer);
            return 0;
        }
        fclose(salesFile);
    }

    if (!isfinite(totalSalesRevenue))
    {
        free(bestSellers);
        free(buffer);
        return 0;
    }

    for (i = 0; i < (int)bestSellerCount; i++)
    {
        if (bestSellers[i].quantitySold > bestSellerQuantity)
        {
            bestSellerQuantity = bestSellers[i].quantitySold;
            bestSellerIndex = i;
        }
    }

    if (!AppendReportText(&buffer, &capacity, &length,
        "PHARMACY MANAGEMENT SYSTEM - GENERATED REPORT\r\n"
        "Report date: %s\r\n"
        "============================================================\r\n\r\n"
        "1. INVENTORY SUMMARY\r\n"
        "Registered medicine records: %d\r\n"
        "Total units currently recorded: %lld\r\n"
        "Current stock value (not expired): %.2f RWF\r\n"
        "Expired stock loss: %.2f RWF\r\n\r\n"
        "2. SALES SUMMARY\r\n"
        "Total sales revenue (after discounts): %.2f RWF\r\n"
        "Number of sales today: %d\r\n",
        today, medicineCount, totalUnits, currentStockValue,
        expiredStockLoss, totalSalesRevenue, salesToday))
    {
        free(bestSellers); free(buffer); return 0;
    }

    if (bestSellerIndex >= 0)
    {
        if (!AppendReportText(&buffer, &capacity, &length,
            "Best-selling medicine: %s (ID: %s)\r\nUnits sold: %lld\r\n",
            bestSellers[bestSellerIndex].medicineName,
            bestSellers[bestSellerIndex].medicineID,
            bestSellers[bestSellerIndex].quantitySold))
        {
            free(bestSellers); free(buffer); return 0;
        }
    }
    else if (!AppendReportText(&buffer, &capacity, &length,
                               "Best-selling medicine: No sales recorded\r\n"))
    {
        free(bestSellers); free(buffer); return 0;
    }

    if (!AppendReportText(&buffer, &capacity, &length,
        "\r\n3. LOW-STOCK MEDICINES (quantity below 10)\r\n"))
    {
        free(bestSellers); free(buffer); return 0;
    }

    {
        int listed = 0;
        for (i = 0; i < medicineCount; i++)
        {
            if (medicines[i].quantity < 10)
            {
                if (!AppendReportText(&buffer, &capacity, &length,
                    "- %s (ID: %s), quantity: %d\r\n",
                    medicines[i].name, medicines[i].id, medicines[i].quantity))
                { free(bestSellers); free(buffer); return 0; }
                listed++;
            }
        }
        if (listed == 0 && !AppendReportText(&buffer, &capacity, &length,
                                             "No low-stock medicines.\r\n"))
        { free(bestSellers); free(buffer); return 0; }
    }

    if (!AppendReportText(&buffer, &capacity, &length,
        "\r\n4. MEDICINES EXPIRING WITHIN 30 DAYS (including today)\r\n"))
    { free(bestSellers); free(buffer); return 0; }

    {
        int listed = 0;
        for (i = 0; i < medicineCount; i++)
        {
            int days = DaysUntilExpiry(medicines[i].expiryDate);
            if (days >= 0 && days <= 30)
            {
                if (!AppendReportText(&buffer, &capacity, &length,
                    "- %s (ID: %s), expiry: %s, days remaining: %d\r\n",
                    medicines[i].name, medicines[i].id,
                    medicines[i].expiryDate, days))
                { free(bestSellers); free(buffer); return 0; }
                listed++;
            }
        }
        if (listed == 0 && !AppendReportText(&buffer, &capacity, &length,
                                             "No medicines expiring within 30 days.\r\n"))
        { free(bestSellers); free(buffer); return 0; }
    }

    if (!AppendReportText(&buffer, &capacity, &length,
        "\r\n5. DATA NOTES\r\n"
        "Expired medicines are excluded from current stock value and counted as expired stock loss.\r\n"
        "Medicines with invalid expiry dates are excluded from both value categories: %d record(s).\r\n"
        "Sales revenue is the final amount paid after any discount.\r\n"
        "Best-selling medicine is based on total units sold across saved sales records.\r\n",
        invalidExpiryCount))
    { free(bestSellers); free(buffer); return 0; }

    free(bestSellers);
    *reportText = buffer;
    return 1;
}

int ExportPharmacyReport(const char *reportText)
{
    FILE *file = fopen("pharmacy_report.txt", "wb");
    size_t length = strlen(reportText);
    int success;

    if (file == NULL)
        return 0;

    success = (fwrite(reportText, 1, length, file) == length);
    if (fclose(file) != 0)
        success = 0;
    return success;
}

LRESULT CALLBACK GenerateReportProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            char *reportText = NULL;
            HWND edit;

            CreateWindow("STATIC", "PHARMACY REPORT",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         300, 10, 330, 28, hwnd, NULL, NULL, NULL);

            edit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE |
                         ES_AUTOVSCROLL | ES_READONLY,
                         15, 45, 900, 555, hwnd, (HMENU)ID_REPORT_TEXT,
                         GetModuleHandle(NULL), NULL);
            SendMessage(edit, EM_SETLIMITTEXT, 0, 0);

            CreateWindow("BUTTON", "Export Report to Text File",
                         WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         350, 615, 220, 35, hwnd, (HMENU)ID_REPORT_EXPORT, NULL, NULL);

            if (!BuildPharmacyReport(&reportText))
            {
                SetDlgItemText(hwnd, ID_REPORT_TEXT,
                    "The report could not be generated. Check the medicine and sales data files, then try again.");
            }
            else
            {
                SetDlgItemText(hwnd, ID_REPORT_TEXT, reportText);
                free(reportText);
            }
            return 0;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_REPORT_EXPORT)
            {
                char *reportText = NULL;
                if (!BuildPharmacyReport(&reportText))
                {
                    MessageBox(hwnd, "The report could not be generated, so no file was exported.",
                               "Report Error", MB_OK | MB_ICONERROR);
                    return 0;
                }
                if (!ExportPharmacyReport(reportText))
                    MessageBox(hwnd, "Could not save pharmacy_report.txt. Check folder permissions and try again.",
                               "Export Error", MB_OK | MB_ICONERROR);
                else
                    MessageBox(hwnd,
                        "Report exported successfully as pharmacy_report.txt in the program's current working folder.",
                        "Export Complete", MB_OK | MB_ICONINFORMATION);
                free(reportText);
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* APPLICATION STARTUP */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR args, int nCmdShow)
{
    INITCOMMONCONTROLSEX icc;

    WNDCLASS wc = {0}, addClass = {0}, viewClass = {0},
             updateClass = {0}, sellClass = {0}, checkClass = {0},
             suppliersClass = {0}, historyClass = {0}, reportClass = {0};

    HWND hwnd;
    MSG msg;

    LoadMedicines();
    LoadSuppliers();

    if (!MigrateSalesFile())
        MessageBox(NULL,
                   "Existing sales could not be migrated. The original sales.dat file has been kept safe.",
                   "Sales Migration Warning", MB_OK | MB_ICONWARNING);

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&icc);

    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = hInst;
    wc.lpszClassName = "PharmacyWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc))
        return 0;

    addClass.lpfnWndProc = AddMedicineProcedure;
    addClass.hInstance = hInst;
    addClass.lpszClassName = "AddMedicineWindow";
    addClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&addClass))
        return 0;

    viewClass.lpfnWndProc = ViewMedicinesProcedure;
    viewClass.hInstance = hInst;
    viewClass.lpszClassName = "ViewMedicinesWindow";
    viewClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&viewClass))
        return 0;

    updateClass.lpfnWndProc = UpdateStockProcedure;
    updateClass.hInstance = hInst;
    updateClass.lpszClassName = "UpdateStockWindow";
    updateClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&updateClass))
        return 0;

    sellClass.lpfnWndProc = SellMedicineProcedure;
    sellClass.hInstance = hInst;
    sellClass.lpszClassName = "SellMedicineWindow";
    sellClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&sellClass))
        return 0;

    /* Register Check Stock Window */
    checkClass.lpfnWndProc = CheckStockProcedure;
    checkClass.hInstance = hInst;
    checkClass.lpszClassName = "CheckStockWindow";
    checkClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&checkClass))
        return 0;

    suppliersClass.lpfnWndProc = ManageSuppliersProcedure;
    suppliersClass.hInstance = hInst;
    suppliersClass.lpszClassName = "ManageSuppliersWindow";
    suppliersClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&suppliersClass))
        return 0;

    historyClass.lpfnWndProc = TransactionHistoryProcedure;
    historyClass.hInstance = hInst;
    historyClass.lpszClassName = "TransactionHistoryWindow";
    historyClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&historyClass))
        return 0;

    reportClass.lpfnWndProc = GenerateReportProcedure;
    reportClass.hInstance = hInst;
    reportClass.lpszClassName = "GenerateReportWindow";
    reportClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&reportClass))
        return 0;

    hwnd = CreateWindowEx(0, "PharmacyWindow", "Pharmacy Management System",
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                          1000, 650, NULL, NULL, hInst, NULL);

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

/* DASHBOARD */
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:

            CreateWindow("STATIC", "PHARMACY MANAGEMENT SYSTEM",
                         WS_VISIBLE | WS_CHILD,
                         300, 40, 400, 40, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Welcome to your pharmacy dashboard",
                         WS_VISIBLE | WS_CHILD,
                         330, 100, 350, 30, hwnd, NULL, NULL, NULL);

            /*
             * DASHBOARD BUTTON GRID
             * All nine buttons use the same size and are arranged in
             * three evenly spaced columns and three rows.
             */

            /* ROW 1 */
            CreateWindow("BUTTON", "Add Medicine",
                         WS_VISIBLE | WS_CHILD,
                         140, 210, 180, 50, hwnd, (HMENU)ID_ADD, NULL, NULL);

            CreateWindow("BUTTON", "View Medicines",
                         WS_VISIBLE | WS_CHILD,
                         410, 210, 180, 50, hwnd, (HMENU)ID_VIEW, NULL, NULL);

            CreateWindow("BUTTON", "Sell Medicine",
                         WS_VISIBLE | WS_CHILD,
                         680, 210, 180, 50, hwnd, (HMENU)ID_SELL, NULL, NULL);

            /* ROW 2 */
            CreateWindow("BUTTON", "Manage Suppliers",
                         WS_VISIBLE | WS_CHILD,
                         140, 300, 180, 50, hwnd, (HMENU)ID_SUPPLIERS, NULL, NULL);

            CreateWindow("BUTTON", "Update Stock",
                         WS_VISIBLE | WS_CHILD,
                         410, 300, 180, 50, hwnd, (HMENU)ID_UPDATE, NULL, NULL);

            CreateWindow("BUTTON", "Check Stock & Expiry",
                         WS_VISIBLE | WS_CHILD,
                         680, 300, 180, 50, hwnd, (HMENU)ID_CHECK, NULL, NULL);

            /* ROW 3 */
            CreateWindow("BUTTON", "Calculate Stock Value",
                         WS_VISIBLE | WS_CHILD,
                         140, 390, 180, 50, hwnd, (HMENU)ID_STOCK_VALUE, NULL, NULL);

            CreateWindow("BUTTON", "Transaction History",
                         WS_VISIBLE | WS_CHILD,
                         410, 390, 180, 50, hwnd, (HMENU)ID_HISTORY, NULL, NULL);

            CreateWindow("BUTTON", "Generate Report",
                         WS_VISIBLE | WS_CHILD,
                         680, 390, 180, 50, hwnd, (HMENU)ID_REPORT, NULL, NULL);

            return 0;

        case WM_COMMAND:

            if (LOWORD(wParam) == ID_ADD)
            {
                HWND w = CreateWindowEx(WS_EX_DLGMODALFRAME,
                                         "AddMedicineWindow", "Add Medicine",
                                         WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                         28, 26, 540, 550,
                                         hwnd, NULL, GetModuleHandle(NULL), NULL);

                if (w)
                    ShowWindow(w, SW_SHOW);
            }

            else if (LOWORD(wParam) == ID_VIEW)
            {
                HWND w = CreateWindowEx(0, "ViewMedicinesWindow", "View Medicines",
                                        WS_OVERLAPPEDWINDOW,
                                        CW_USEDEFAULT, CW_USEDEFAULT, 1050, 520,
                                        hwnd, NULL, GetModuleHandle(NULL), NULL);

                if (w)
                    ShowWindow(w, SW_SHOW);
            }

            else if (LOWORD(wParam) == ID_SUPPLIERS)
            {
                HWND w = CreateWindowEx(0, "ManageSuppliersWindow", "Manage Suppliers",
                                        WS_OVERLAPPEDWINDOW,
                                        CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
                                        hwnd, NULL, GetModuleHandle(NULL), NULL);

                if (w)
                    ShowWindow(w, SW_SHOW);
            }

            else if (LOWORD(wParam) == ID_UPDATE)
            {
                HWND w = CreateWindowEx(WS_EX_DLGMODALFRAME,
                                        "UpdateStockWindow", "Update Stock",
                                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        550, 350, hwnd, NULL,
                                        GetModuleHandle(NULL), NULL);

                if (w)
                    ShowWindow(w, SW_SHOW);
            }

            else if (LOWORD(wParam) == ID_SELL)
            {
                HWND w = CreateWindowEx(WS_EX_DLGMODALFRAME,
                                        "SellMedicineWindow", "Sell Medicine",
                                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        560, 455, hwnd, NULL,
                                        GetModuleHandle(NULL), NULL);

                if (w)
                    ShowWindow(w, SW_SHOW);
            }

            /* OPEN CHECK STOCK WINDOW */
            else if (LOWORD(wParam) == ID_CHECK)
            {
                HWND w = CreateWindowEx(WS_EX_DLGMODALFRAME,
                                        "CheckStockWindow", "Check Stock and Expiry",
                                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                        CW_USEDEFAULT, CW_USEDEFAULT, 550, 440,
                                        hwnd, NULL, GetModuleHandle(NULL), NULL);

                if (w)
                    ShowWindow(w, SW_SHOW);
            }

            /* CALCULATE STOCK VALUE */
            else if (LOWORD(wParam) == ID_STOCK_VALUE)
            {
                HandleCalculateStockValue(hwnd);
            }

            else if (LOWORD(wParam) == ID_HISTORY)
            {
                HWND w = CreateWindowEx(0, "TransactionHistoryWindow", "Transaction History",
                                        WS_OVERLAPPEDWINDOW,
                                        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 650,
                                        hwnd, NULL, GetModuleHandle(NULL), NULL);
                if (w)
                    ShowWindow(w, SW_SHOW);
            }

            else if (LOWORD(wParam) == ID_REPORT)
            {
                HWND w = CreateWindowEx(0, "GenerateReportWindow", "Generate Pharmacy Report",
                                        WS_OVERLAPPEDWINDOW,
                                        CW_USEDEFAULT, CW_USEDEFAULT, 950, 720,
                                        hwnd, NULL, GetModuleHandle(NULL), NULL);
                if (w)
                    ShowWindow(w, SW_SHOW);
            }

            return 0;

        case WM_DESTROY:

            PostQuitMessage(0);
            return 0;

        default:

            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* ADD MEDICINE WINDOW */
LRESULT CALLBACK AddMedicineProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            HWND category, prescription;

            CreateWindow("STATIC", "MEDICINE DETAILS",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         150, 15, 220, 30, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Medicine ID:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 55, 130, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         180, 55, 300, 25, hwnd, (HMENU)101, NULL, NULL);

            CreateWindow("STATIC", "Medicine Name:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 90, 130, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         180, 90, 300, 25, hwnd, (HMENU)102, NULL, NULL);

            CreateWindow("STATIC", "Category:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 125, 130, 25, hwnd, NULL, NULL, NULL);

            category = CreateWindow("COMBOBOX", "",
                                    WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                    180, 125, 300, 120, hwnd, (HMENU)103, NULL, NULL);

            SendMessage(category, CB_ADDSTRING, 0, (LPARAM)"Tablet");
            SendMessage(category, CB_ADDSTRING, 0, (LPARAM)"Syrup");
            SendMessage(category, CB_ADDSTRING, 0, (LPARAM)"Injection");
            SendMessage(category, CB_ADDSTRING, 0, (LPARAM)"Ointment");

            CreateWindow("STATIC", "Unit Price (RWF):",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 160, 130, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         180, 160, 300, 25, hwnd, (HMENU)104, NULL, NULL);

            CreateWindow("STATIC", "Quantity:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 195, 130, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         180, 195, 300, 25, hwnd, (HMENU)105, NULL, NULL);

            CreateWindow("STATIC", "Batch Number:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 230, 130, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         180, 230, 300, 25, hwnd, (HMENU)106, NULL, NULL);

            CreateWindow("STATIC", "Manufacture Date:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 265, 130, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "YYYY-MM-DD",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         180, 265, 300, 25, hwnd, (HMENU)107, NULL, NULL);

            CreateWindow("STATIC", "Expiry Date:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 300, 130, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "YYYY-MM-DD",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         180, 300, 300, 25, hwnd, (HMENU)108, NULL, NULL);

            CreateWindow("STATIC", "Prescription Required:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 335, 145, 25, hwnd, NULL, NULL, NULL);

            prescription = CreateWindow("COMBOBOX", "",
                                        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                        180, 335, 300, 100, hwnd, (HMENU)109, NULL, NULL);

            SendMessage(prescription, CB_ADDSTRING, 0, (LPARAM)"Yes");
            SendMessage(prescription, CB_ADDSTRING, 0, (LPARAM)"No");

            CreateWindow("BUTTON", "Save",
                         WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         170, 395, 120, 35, hwnd, (HMENU)111, NULL, NULL);

            CreateWindow("BUTTON", "Cancel",
                         WS_VISIBLE | WS_CHILD,
                         310, 395, 120, 35, hwnd, (HMENU)112, NULL, NULL);

            return 0;
        }

        case WM_COMMAND:

            if (LOWORD(wParam) == 111)
                HandleSaveMedicine(hwnd);

            else if (LOWORD(wParam) == 112)
                DestroyWindow(hwnd);

            return 0;

        case WM_CLOSE:

            DestroyWindow(hwnd);
            return 0;

        default:

            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* VIEW MEDICINES */
LRESULT CALLBACK ViewMedicinesProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND list;

    switch (msg)
    {
        case WM_CREATE:

            CreateWindow("STATIC", "REGISTERED MEDICINES",
                         WS_VISIBLE | WS_CHILD,
                         20, 15, 300, 30, hwnd, NULL, NULL, NULL);

            list = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
                                  WS_VISIBLE | WS_CHILD | LVS_REPORT |
                                  LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                  15, 55, 1000, 330, hwnd, (HMENU)201,
                                  GetModuleHandle(NULL), NULL);

            ListView_SetExtendedListViewStyle(list,
                                              LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            AddListColumn(list, 0, "ID", 100);
            AddListColumn(list, 1, "Name", 180);
            AddListColumn(list, 2, "Category", 110);
            AddListColumn(list, 3, "Quantity", 90);
            AddListColumn(list, 4, "Unit Price", 110);
            AddListColumn(list, 5, "Batch No.", 130);
            AddListColumn(list, 6, "Expiry", 120);

            FillMedicineTable(list);

            CreateWindow("BUTTON", "Remove Selected Medicine",
                         WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         380, 400, 220, 35, hwnd, (HMENU)ID_REMOVE_MEDICINE, NULL, NULL);

            CreateWindow("STATIC",
                         "Select a row, then remove it. Expired medicines can be removed here after confirmation.",
                         WS_VISIBLE | WS_CHILD,
                         20, 440, 850, 22, hwnd, NULL, NULL, NULL);

            return 0;

        case WM_COMMAND:

            if (LOWORD(wParam) == ID_REMOVE_MEDICINE)
                HandleRemoveSelectedMedicine(hwnd, list);

            return 0;

        case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->idFrom == 201 && header->code == NM_CUSTOMDRAW)
            {
                LPNMLVCUSTOMDRAW draw = (LPNMLVCUSTOMDRAW)lParam;

                switch (draw->nmcd.dwDrawStage)
                {
                    case CDDS_PREPAINT:

                        return CDRF_NOTIFYITEMDRAW;

                    case CDDS_ITEMPREPAINT:
                    {
                        int index = (int)draw->nmcd.dwItemSpec;

                        if (index >= 0 && index < medicineCount)
                        {
                            int days = DaysUntilExpiry(medicines[index].expiryDate);

                            if (days < 0)
                            {
                                draw->clrTextBk = RGB(190, 190, 190);
                                draw->clrText = RGB(70, 70, 70);
                            }

                            else if (days <= 30)
                            {
                                draw->clrTextBk = RGB(255, 210, 150);
                                draw->clrText = RGB(0, 0, 0);
                            }

                            else if (medicines[index].quantity < 10)
                            {
                                draw->clrTextBk = RGB(255, 170, 170);
                                draw->clrText = RGB(0, 0, 0);
                            }
                        }

                        return CDRF_NEWFONT;
                    }
                }
            }

            break;
        }

        case WM_CLOSE:

            DestroyWindow(hwnd);
            return 0;

        default:

            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* UPDATE STOCK WINDOW */
LRESULT CALLBACK UpdateStockProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:

            CreateWindow("STATIC", "UPDATE MEDICINE STOCK",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         100, 15, 330, 30, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Medicine ID:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 65, 140, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         190, 65, 290, 25, hwnd, (HMENU)301, NULL, NULL);

            CreateWindow("STATIC", "Additional Quantity:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 105, 140, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         190, 105, 290, 25, hwnd, (HMENU)302, NULL, NULL);

            CreateWindow("STATIC", "New Batch Number:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 145, 140, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         190, 145, 290, 25, hwnd, (HMENU)303, NULL, NULL);

            CreateWindow("STATIC", "New Expiry Date:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 185, 140, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "YYYY-MM-DD",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         190, 185, 290, 25, hwnd, (HMENU)304, NULL, NULL);

            CreateWindow("BUTTON", "Update Stock",
                         WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         125, 245, 130, 35, hwnd, (HMENU)305, NULL, NULL);

            CreateWindow("BUTTON", "Cancel",
                         WS_VISIBLE | WS_CHILD,
                         275, 245, 130, 35, hwnd, (HMENU)306, NULL, NULL);

            return 0;

        case WM_COMMAND:

            if (LOWORD(wParam) == 305)
                HandleUpdateStock(hwnd);

            else if (LOWORD(wParam) == 306)
                DestroyWindow(hwnd);

            return 0;

        case WM_CLOSE:

            DestroyWindow(hwnd);
            return 0;

        default:

            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* SELL MEDICINE WINDOW */
LRESULT CALLBACK SellMedicineProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            HWND reasonCombo;

            CreateWindow("STATIC", "SELL MEDICINE",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         130, 15, 280, 30, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Medicine ID:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 60, 150, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         200, 60, 290, 25, hwnd, (HMENU)401, NULL, NULL);

            CreateWindow("STATIC", "Customer Name:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 95, 150, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         200, 95, 290, 25, hwnd, (HMENU)402, NULL, NULL);

            CreateWindow("STATIC", "Quantity to Sell:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 130, 150, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         200, 130, 290, 25, hwnd, (HMENU)403, NULL, NULL);

            CreateWindow("STATIC", "Prescription Number:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 165, 160, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         200, 165, 290, 25, hwnd, (HMENU)404, NULL, NULL);

            CreateWindow("STATIC", "Discount (%):",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 205, 150, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         200, 205, 290, 25, hwnd, (HMENU)407, NULL, NULL);

            CreateWindow("STATIC", "Discount Reason:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 240, 150, 25, hwnd, NULL, NULL, NULL);

            reasonCombo = CreateWindow("COMBOBOX", "",
                                       WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                       200, 240, 290, 100, hwnd, (HMENU)408, NULL, NULL);

            SendMessage(reasonCombo, CB_ADDSTRING, 0, (LPARAM)"Senior citizen");
            SendMessage(reasonCombo, CB_ADDSTRING, 0, (LPARAM)"Insurance");
            SendMessage(reasonCombo, CB_ADDSTRING, 0, (LPARAM)"Bulk purchase");

            CreateWindow("BUTTON", "Complete Sale",
                         WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         125, 300, 140, 35, hwnd, (HMENU)405, NULL, NULL);

            CreateWindow("BUTTON", "Cancel",
                         WS_VISIBLE | WS_CHILD,
                         285, 300, 120, 35, hwnd, (HMENU)406, NULL, NULL);

            CreateWindow("STATIC",
                         "Discount is optional. If used, enter 5-15% and choose a reason.",
                         WS_VISIBLE | WS_CHILD,
                         30, 350, 490, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC",
                         "Prescription number is only required when the medicine needs one.",
                         WS_VISIBLE | WS_CHILD,
                         30, 375, 490, 25, hwnd, NULL, NULL, NULL);

            return 0;
        }

        case WM_COMMAND:

            if (LOWORD(wParam) == 405)
                HandleSellMedicine(hwnd);

            else if (LOWORD(wParam) == 406)
                DestroyWindow(hwnd);

            return 0;

        case WM_CLOSE:

            DestroyWindow(hwnd);
            return 0;

        default:

            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* CHECK STOCK WINDOW */
LRESULT CALLBACK CheckStockProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:

            CreateWindow("STATIC", "CHECK STOCK AND EXPIRY",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         100, 15, 340, 30, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Medicine ID:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         30, 65, 130, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         170, 65, 280, 25, hwnd, (HMENU)501, NULL, NULL);

            CreateWindow("BUTTON", "Check Medicine",
                         WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         170, 105, 140, 35, hwnd, (HMENU)502, NULL, NULL);

            CreateWindow("BUTTON", "Clear",
                         WS_VISIBLE | WS_CHILD,
                         320, 105, 100, 35, hwnd, (HMENU)503, NULL, NULL);

            CreateWindow("STATIC", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER,
                         30, 155, 470, 220, hwnd, (HMENU)504, NULL, NULL);

            return 0;

        case WM_COMMAND:

            if (LOWORD(wParam) == 502)
                HandleCheckStock(hwnd);

            else if (LOWORD(wParam) == 503)
            {
                SetDlgItemText(hwnd, 501, "");
                SetDlgItemText(hwnd, 504, "");
            }

            return 0;

        case WM_CLOSE:

            DestroyWindow(hwnd);
            return 0;

        default:

            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* MANAGE SUPPLIERS WINDOW */
LRESULT CALLBACK ManageSuppliersProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            HWND list;

            CreateWindow("STATIC", "REGISTERED SUPPLIERS",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         250, 12, 350, 28, hwnd, NULL, NULL, NULL);

            list = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
                                  WS_VISIBLE | WS_CHILD | LVS_REPORT |
                                  LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                  15, 45, 850, 250, hwnd, (HMENU)ID_SUPPLIER_LIST,
                                  GetModuleHandle(NULL), NULL);

            ListView_SetExtendedListViewStyle(list,
                                              LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            AddListColumn(list, 0, "Supplier ID", 120);
            AddListColumn(list, 1, "Supplier Name", 190);
            AddListColumn(list, 2, "Phone", 150);
            AddListColumn(list, 3, "Medicines Supplied", 365);
            FillSupplierTable(list);

            CreateWindow("STATIC", "SUPPLIER DETAILS",
                         WS_VISIBLE | WS_CHILD,
                         25, 310, 250, 25, hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Supplier ID:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         25, 345, 125, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         155, 345, 270, 25, hwnd, (HMENU)ID_SUPPLIER_ID, NULL, NULL);

            CreateWindow("STATIC", "Supplier Name:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         455, 345, 125, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         585, 345, 270, 25, hwnd, (HMENU)ID_SUPPLIER_NAME, NULL, NULL);

            CreateWindow("STATIC", "Phone:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         25, 385, 125, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         155, 385, 270, 25, hwnd, (HMENU)ID_SUPPLIER_PHONE, NULL, NULL);

            CreateWindow("STATIC", "Medicines Supplied:",
                         WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE,
                         455, 385, 125, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         585, 385, 270, 25, hwnd, (HMENU)ID_SUPPLIER_MEDICINES, NULL, NULL);

            CreateWindow("BUTTON", "Add Supplier",
                         WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         230, 445, 140, 35, hwnd, (HMENU)ID_SUPPLIER_ADD, NULL, NULL);
            CreateWindow("BUTTON", "Remove Selected",
                         WS_VISIBLE | WS_CHILD,
                         390, 445, 150, 35, hwnd, (HMENU)ID_SUPPLIER_REMOVE, NULL, NULL);
            CreateWindow("BUTTON", "Clear Fields",
                         WS_VISIBLE | WS_CHILD,
                         560, 445, 130, 35, hwnd, (HMENU)ID_SUPPLIER_CLEAR, NULL, NULL);

            CreateWindow("STATIC",
                         "Supplier records are saved automatically and loaded when the program starts.",
                         WS_VISIBLE | WS_CHILD,
                         25, 500, 800, 22, hwnd, NULL, NULL, NULL);

            return 0;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_SUPPLIER_ADD)
                HandleAddSupplier(hwnd);
            else if (LOWORD(wParam) == ID_SUPPLIER_REMOVE)
                HandleRemoveSupplier(hwnd);
            else if (LOWORD(wParam) == ID_SUPPLIER_CLEAR)
            {
                SetDlgItemText(hwnd, ID_SUPPLIER_ID, "");
                SetDlgItemText(hwnd, ID_SUPPLIER_NAME, "");
                SetDlgItemText(hwnd, ID_SUPPLIER_PHONE, "");
                SetDlgItemText(hwnd, ID_SUPPLIER_MEDICINES, "");
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

